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
#include "p2p/msg.h"
#include "core/log.h"


const std::vector<std::string> P2P::kBootStrap = {}; //{{"127.0.0.1:123"}};

bool P2P::start(){
    mLog.d("Starting P2P network");
    std::unique_lock<std::mutex> lock(mThreadMutex);

    if (mRunning){
        mLog.d("P2P network already running");
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
        mLog.e("getaddrinfo {}", errno);
        return false;
    }

    // Create the socket
    mMainSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (mMainSocket == -1) {
        mLog.e("socket creation error");
        return false;
    }

    // Enable the socket to reuse the address
    if (setsockopt(mMainSocket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
        mLog.e("setsockopt {}", errno);
        return false;
    }

    // Bind to the address
    if (bind(mMainSocket, res->ai_addr, res->ai_addrlen) == -1) {
        mLog.e("bind {}", errno);
        return false;
    }

    // Listen
    if (listen(mMainSocket, kListenQueueLen) == -1) {
        mLog.e("listen {}", errno);
        return false;
    }
    freeaddrinfo(res);

    mLog.i("opened listen socket on port {}", mListenPort);

    // Event Fd to send data to the thread
    mEventFd = eventfd(0,0);
    if(mEventFd == -1){
        mLog.e("Eventfd not set up {}", errno);
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
        aConnect(str);
    }

    return true;
}

void P2P::stop(){
    std::unique_lock<std::mutex> lock1(mTasksMutex, std::defer_lock);
    std::unique_lock<std::mutex> lock2(mThreadMutex, std::defer_lock);
    std::lock(lock1, lock2);
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

void P2P::aConnect(const std::string& address) {
    std::unique_lock<std::mutex> lock(mTasksMutex);
    mTasks.emplace_back(std::async(std::launch::async, &P2P::connect, this, address));
}

int P2P::connect(std::string address) {
    mLog.t("connecting to {}", address);

    //Split by ":"
    std::vector<char> chars(address.c_str(), address.c_str() + address.size() + 1u);
    char * addr = std::strtok(chars.data(), ":");
    auto port = atoi(std::strtok(NULL, ":"));

    auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int sock = 0, valread;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        mLog.e("Socket creation error");
        return -1;
    }

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, addr, &sockaddr.sin_addr)<=0)
    {
        mLog.w("Invalid address {}", addr);
        return -2;
    }
    if (::connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        mLog.w("Connection Failed to {}:{}",
            inet_ntoa(sockaddr.sin_addr), htons(sockaddr.sin_port));
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

    // Send our welcome msg
    msgpack::sbuffer packed;
    msgpack::zone z;
    Msg::Any msg = {Msg::Type::WELCOME, 
        msgpack::object(Msg::Welcome{0,"DANI"}, z)};
    msgpack::pack(&packed, msg);
    if (packed.size() != write(sock, packed.data(), packed.size())) {
        mLog.e("Error sending to socket");
    }

    return 0;
}

void P2P::newClient(struct epoll_event& ev) {
    //New connection in Main socket
    socklen_t size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    int newsock = accept(mMainSocket, (struct sockaddr*)&addr, &size);
    if (newsock == -1) {
        mLog.e("accept error {}", errno);
    } else {
        mLog.i("Got a connection (fd {}) from {}:{}",
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
        mLog.e("Serve client requested on unknown FD={}", fd);
        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, NULL);
    }
    auto& client = it->second;
    std::unique_lock<std::mutex> clientLock(client.mMutex);
    lock.unlock();

    auto& unp = client.mUnpacker;
    unp.reserve_buffer(kUpackBuffer);
    int valread = -1;
    if ((valread = read(fd, unp.buffer(), kUpackBuffer)) <= 0){
        if (valread < 0) {
            mLog.w("Socket returned {}", valread);
        }
        // Disconnected
        auto& addr = client.mSockAddr;
        mLog.i("Disconnected (fd {}) from {}:{}",
                fd, inet_ntoa(addr.sin_addr), htons(addr.sin_port));

        // Close the socket and remove from poll
        close(fd);
        clientLock.unlock();
        mClients.erase(it);
    } else {
        //Packet on already connected client
        mLog.t("Packet on connected client (size {})", valread);

        unp.buffer_consumed(valread);
        msgpack::object_handle result;
        // Message pack data loop
        while(unp.next(result)) {
            msgpack::object obj(result.get());
            
            Msg::Any any;
            obj.convert(any);
            if (any.type == Msg::Type::WELCOME) {
                Msg::Welcome msg;
                any.data.convert(msg);
                mLog.e("Welcome msg received {}",msg.mInfo.mName.data());
            } else if (any.type == Msg::Type::PEER_INFO) {
                Msg::PeerInfo msg;
                any.data.convert(msg);
                mLog.e("Peer msg received {}", msg.mName.data());
            }
        }
    }
}

void P2P::handleEvents() {
    Event ev;
    read(mEventFd, &ev, sizeof(ev));
    switch(ev) {
        default:
            mLog.e("unknown event received {}", ev);
        case SHUTDOWN:
            mLog.d("Event received {}", ev);
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
    mLog.t("thread created");

    // Add Main Socket and Event Fd
    epollCtl(EPOLL_CTL_ADD, mMainSocket, EPOLLIN, -1);
    epollCtl(EPOLL_CTL_ADD, mEventFd, EPOLLIN, -2);

    // Main loop
    while (mRunning) {
        struct epoll_event event;
        if(epoll_wait(mEpollFd, &event, 1, -1) == -1){
            mLog.e("epoll error {}", errno);
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

    mLog.t("thread stopped");
}

int P2P::getNumClients(){
    std::unique_lock<std::mutex> lock(mClientMutex);

    return mClients.size();
}
