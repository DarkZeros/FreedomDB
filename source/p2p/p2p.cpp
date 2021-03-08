#include <stdio.h>
#include <string.h> // memset()
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "p2p/p2p.h"
#include "core/log.h"


const std::vector<std::string> P2P::kBootStrap = {}; //{{"127.0.0.1:123"}};

bool P2P::start(){
    std::unique_lock<std::mutex> lock(mThreadMutex);
    Log::log(Log::Type::P2P, Log::Level::DEBUG, "Starting P2P network");

    if (mRunning){
        Log::log(Log::Type::P2P, Log::Level::INFO, "P2P network already running");
        return true; //Its ok
    }

    // We first do the initialization of socket in the main thread
    //  This make seasy to report errors
    // If everything in the setup succeeds, then start the thread loop

    struct addrinfo hints, *res;
    int reuseaddr = 1;

    // Get the address info
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(NULL, fmt::format("{}", mListenPort).c_str(), &hints, &res) != 0) {
        Log::log(Log::Type::P2P, Log::Level::ERROR, "getaddrinfo %d", errno);
        return false;
    }

    // Create the socket
    mMainSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (mMainSocket == -1) {
        Log::log(Log::Type::P2P, Log::Level::ERROR, "socket");
        return false;
    }

    // Enable the socket to reuse the address
    if (setsockopt(mMainSocket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
        Log::log(Log::Type::P2P, Log::Level::ERROR, "setsockopt %d", errno);
        return false;
    }

    // Bind to the address
    if (bind(mMainSocket, res->ai_addr, res->ai_addrlen) == -1) {
        Log::log(Log::Type::P2P, Log::Level::ERROR, "bind %d", errno);
        return false;
    }

    // Listen
    if (listen(mMainSocket, kListenQueueLen) == -1) {
        Log::log(Log::Type::P2P, Log::Level::ERROR, "listen %d", errno);
        return false;
    }
    freeaddrinfo(res);

    Log::log(Log::Type::P2P, Log::Level::INFO, "opened listen socket on port %d", mListenPort);

    // Event Fd to send data to the thread
    mEventFd = eventfd(0,0);
    if(mEventFd == -1){
        Log::log(Log::Type::P2P, Log::Level::ERROR, "Eventfd not set up %d", errno);
        return false;
    }

    // Initialize epoll that the thread will serve
    //  we do this before the thread to be able to start straight away
    //  connecting and adding clients before we even serve it
    mEpollFd = epoll_create(1);

    // Launch thread
    std::unique_lock<std::mutex> lock2(mClientMutex);
    mClients.clear();
    mThread = std::thread(&P2P::threadLoop, this);
    mRunning = true;

    // Launch connections async on all bootstrap addresses
    for (auto& str : mBootStrap) {
        mTasks.emplace_back(std::async(std::launch::async, &P2P::connect, this, str));
    }

    Log::log(Log::Type::P2P, Log::Level::ERROR, "Done");

    return true;
}

void P2P::stop(){
    std::unique_lock<std::mutex> lock(mThreadMutex);
    mTasks.clear();
    if (mRunning){
        //Send signal to thread via signalfd
        uint64_t u64 = 1;
        write(mEventFd, &u64, sizeof(uint64_t));
        mThread.join();
        close(mEventFd);
        mEventFd = -1;
    }
    mRunning = false;
}

int P2P::connect(std::string address) {
    Log::log(Log::Type::P2P, Log::Level::INFO, "connecting to %s", address.c_str());

    //Split by ":"
    std::vector<char> chars(address.c_str(), address.c_str() + address.size() + 1u);
    char * addr = std::strtok(chars.data(), ":");
    auto port = atoi(std::strtok(NULL, ":"));

    auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int sock = 0, valread;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        Log::log(Log::Type::P2P, Log::Level::INFO, "Socket creation error");
        return -1;
    }

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, addr, &sockaddr.sin_addr)<=0)
    {
        Log::log(Log::Type::P2P, Log::Level::INFO, "Invalid address/ Address not supported");
        return -2;
    }
    if (::connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        Log::log(Log::Type::P2P, Log::Level::INFO, "Connection Failed");
        return -3;
    }

    // At this point the connection succeeded add it to the queue of mClients
    {
        std::unique_lock<std::mutex> lock(mClientMutex);
        mClients[sock].mSockAddr = sockaddr;
    }
    {
        std::unique_lock<std::mutex> lock(mThreadMutex);
        epollCtl(EPOLL_CTL_ADD, sock, EPOLLIN, sock);
    }

    // Send welcome msg

    return 0;
}

void P2P::newClient(struct epoll_event& ev) {
    //New connection in Main socket
    socklen_t size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    int newsock = accept(mMainSocket, (struct sockaddr*)&addr, &size);
    if (newsock == -1) {
        Log::log(Log::Type::P2P, Log::Level::ERROR, "accept error %d", errno);
    } else {
        Log::log(Log::Type::P2P, Log::Level::INFO, "Got a connection (id %d) from %s on port %d",
                newsock, inet_ntoa(addr.sin_addr), htons(addr.sin_port));
        //Add it to the client socket list & epoll
        std::unique_lock<std::mutex> lock(mClientMutex);
        mClients[newsock].mSockAddr = addr;
        ev.events = EPOLLIN;
        ev.data.fd = newsock;
        epoll_ctl(mEpollFd, EPOLL_CTL_ADD, newsock, &ev);
    }
}

void P2P::serveClient(int fd) {
    std::unique_lock<std::mutex> lock(mClientMutex);
    auto it = mClients.find(fd);
    if (it == mClients.end()) {
        Log::log(Log::Type::P2P, Log::Level::ERROR, "Serve client requested on unknown FD=%d", fd);
        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, NULL);
    }
    auto& client = it->second;
    std::unique_lock<std::mutex> clientLock(client.mMutex);
    lock.unlock();

    auto& buffer = client.mRxBuf;
    buffer.resize(buffer.size()+4096); //Extend it by 4096
    int valread = -1;
    if ((valread = read(fd, buffer.data()+buffer.size()-4096, 4096)) == 0){
        // Disconnected
        auto& addr = client.mSockAddr;
        Log::log(Log::Type::P2P, Log::Level::INFO, "Disconnected (id %d) from %s on port %d",
                fd, inet_ntoa(addr.sin_addr), htons(addr.sin_port));

        // Close the socket and remove from poll
        close(fd);
        clientLock.unlock();
        mClients.erase(it);
    }else{
        //Packet on already connected client
        Log::log(Log::Type::P2P, Log::Level::DEBUG, "Packet on connected client (size %d)", valread);

        buffer.resize(buffer.size()-4096+valread); //Shrink it again

        //Process it
        //TODO

        //For the time being just print it
        Log::log(Log::Type::P2P, Log::Level::DEBUG, "Client: %s", buffer.data());
    }
}

void P2P::handleEvents() {
    Event ev;
    read(mEventFd, &ev, sizeof(ev));
    switch(ev) {
        default:
            Log::log(Log::Type::P2P, Log::Level::ERROR, "unknown event received");
        case SHUTDOWN:
            Log::log(Log::Type::P2P, Log::Level::DEBUG, "SHUTDOWN event received");
            mRunning = false;
    }
}

void P2P::epollCtl(int op, int fd, uint32_t ev, int data) {
    struct epoll_event event;
    event.events = ev;
    event.data.fd = data;
    epoll_ctl(mEpollFd, op, fd, &event);
}

void P2P::threadLoop(void) {
    Log::log(Log::Type::P2P, Log::Level::INFO, "thread created");

    // Add Main Socket and Event Fd
    epollCtl(EPOLL_CTL_ADD, mMainSocket, EPOLLIN, -1);
    epollCtl(EPOLL_CTL_ADD, mEventFd, EPOLLIN, -2);

    // Main loop
    while (mRunning) {
        struct epoll_event event;
        if(epoll_wait(mEpollFd, &event, 1, -1) == -1){
            Log::log(Log::Type::P2P, Log::Level::ERROR, "epoll error %d", errno);
            break;
        }

        // We have been woken, check what to attend
        if (event.data.fd == -2) {
            // Event socket
            handleEvents();
        } else if (event.data.fd == -1){
            //Process the main socket socket that has data
            newClient(event);
        } else {
            // Serve this socket
            serveClient(event.data.fd);
        }
    }

    //Stop epoll
    close(mEpollFd);

    //Stop all sockets
    {
        std::unique_lock<std::mutex> lock(mClientMutex);
        for(auto& i : mClients){
            close(i.first);
        }
        mClients.clear();
    }

    close(mMainSocket);
    mMainSocket = -1;

    Log::log(Log::Type::P2P, Log::Level::INFO, "thread stopped");
}

int P2P::getNumClients(){
    std::unique_lock<std::mutex> lock(mClientMutex);

    return mClients.size();
}
