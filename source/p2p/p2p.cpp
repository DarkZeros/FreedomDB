#include <stdio.h>
#include <string.h> // memset()
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <random>

#include <fmt/format.h>

#include "p2p/p2p.h"
#include "p2p/msg.h"
#include "core/log.h"


const std::vector<std::string> P2P::kBootStrap = {}; //{{"127.0.0.1:123"}};
std::atomic<int> P2P::mUID = [](){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib;
    return distrib(gen);
}();

bool P2P::start(){
    mLog.d("Starting P2P network");
    std::unique_lock lock(mThreadMutex);

    if (mRunning){
        mLog.d("P2P network already running");
        return true; //Its ok
    }

    // Set our own peer info to inform the others (this can be changed)
    mOwnPeerInfo.mVersion = 0; // TODO: Take from global config
    mOwnPeerInfo.mNetID = 0; // TODO: Take from global config
    mOwnPeerInfo.mListenPort = mListenPort;
    mOwnPeerInfo.mUID = mUID++;

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
    if(pipe(mEventPipe)){
        mLog.e("Pipe with thread not set up {}", errno);
        return false;
    }

    // Initialize epoll that the thread will serve
    //  we do this before the thread to be able to start straight away
    //  connecting and adding clients before we even serve it
    mEpollFd = epoll_create(1);

    // Launch thread
    std::unique_lock lock2(mPeersMutex);
    mPeers.clear();
    mRunning = true;
    mThread = std::thread(&P2P::threadLoop, this);

    // Launch connections async on all bootstrap addresses
    for (auto& str : mBootStrap) {
        aConnect(str);
    }

    return true;
}

void P2P::stop(){
    std::unique_lock lock1(mTasksMutex, std::defer_lock);
    std::unique_lock lock2(mThreadMutex, std::defer_lock);
    std::lock(lock1, lock2);
    mTasks.clear();
    if (mRunning){
        //Send Event to thread
        sendThreadEvent(Event::SHUTDOWN);
        mThread.join();
    }
    mRunning = false;
}

void P2P::sendThreadEvent(const Event& ev) {
    if (sizeof(ev) != write(mEventPipe[1], &ev, sizeof(ev))) {
        mLog.e("Error sending event to Thread");
    }
}


void P2P::aConnect(const std::string& address) {
    std::unique_lock lock(mTasksMutex);
    mTasks.emplace_back(std::async(std::launch::async, &P2P::connect, this, address));
}

int P2P::connect(const std::string& address) {
    mLog.t("connecting to '{}'", address);

    auto pos = address.find(':');
    if (pos == address.npos) {
        mLog.w("No ':' found in address '{}'", address);
        return -1;
    }
    auto addr = address.substr(0, pos);
    auto p = address.substr(pos+1);
    auto port = atoi(p.c_str());

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
    if (inet_pton(AF_INET, addr.c_str(), &sockaddr.sin_addr)<=0)
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

    // At this point the connection succeeded add it to the queue of mPeers
    std::unique_lock lock(mPeersMutex);
    auto& peer = insertPeer(sockaddr, sock, Peer::Direction::OUT);

    // We connected, so we have to send our own peer info/discovery list
    sendThreadEvent(PEER_WELCOME);
    sendThreadEventData(sock);

    return 0;
}

Peer& P2P::insertPeer(const sockaddr_in& sockaddr, int sock, const Peer::Direction& dir) {
    {
        std::unique_lock lock(mPeersMutex);
        mPeers[sock].mConAddress = inet_ntoa(sockaddr.sin_addr);
        mPeers[sock].mConPort = htons(sockaddr.sin_port);
        mPeers[sock].mFd = sock;
        mPeers[sock].mReady = false; // Not ready until we check peer info
        mPeers[sock].mDirection = dir;
    }
    {
        std::unique_lock<std::mutex> lock(mThreadMutex);
        if (mRunning)
            epollCtl(EPOLL_CTL_ADD, sock, EPOLLIN, sock);
    }
    return mPeers[sock];
}

void P2P::newPeer(struct epoll_event& ev) {
    // New connection in Main socket
    socklen_t size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    int newsock = accept(mMainSocket, (struct sockaddr*)&addr, &size);
    if (newsock == -1) {
        mLog.e("accept error {}", errno);
    } else {
        mLog.i("Got a connection (fd {}) from {}:{}",
                newsock, inet_ntoa(addr.sin_addr), htons(addr.sin_port));
        // Add it to the peer socket list & epoll
        insertPeer(addr, newsock, Peer::Direction::IN);
    }
}

void P2P::removePeer(const Peer& peer) {
    removePeer(peer.mFd);
}
void P2P::removePeer(int fd) {
    std::unique_lock lock(mPeersMutex);
    auto it = mPeers.find(fd);
    if (it != mPeers.end()) {
        auto& peer = it->second;
        std::unique_lock lock(peer.mMutex);
        lock.unlock();

        // Disconnected
        mLog.i("Disconnected from {}", peer);

        // Close the socket and remove from poll
        close(peer.mFd);
        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, peer.mFd, NULL);
        mPeers.erase(it);
    }
}


void P2P::servePeer(int fd) {
    std::unique_lock lock(mPeersMutex);
    auto it = mPeers.find(fd);
    if (it == mPeers.end()) {
        mLog.w("Serve peer requested on unknown FD={}", fd);
        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, NULL);
        return;
    }
    auto& peer = it->second;

    std::unique_lock peerLock(peer.mMutex);
    lock.unlock();

    auto& unp = peer.mUnpacker;
    unp.reserve_buffer(kUpackBuffer);
    int valread = -1;
    if ((valread = read(fd, unp.buffer(), kUpackBuffer)) <= 0){
        if (valread < 0) {
            mLog.w("Socket returned {}", valread);
        }
        peerLock.unlock();
        removePeer(fd);
    } else {
        //Packet on already connected peer
        mLog.t("Packet on connected peer (size {})", valread);

        unp.buffer_consumed(valread);
        msgpack::object_handle result;
        // Message pack data loop, only one, since it may be destroyed afterwards
        if (unp.next(result)) {
            msgpack::object obj(result.get());
            
            // New object received, parse it to the peer processor
            decodeMsg(peer, obj);
        }
    }
}

void P2P::handleEvents() {
    Event ev;
    read(mEventPipe[0], &ev, sizeof(ev));
    switch(ev) {
        default:
            mLog.e("unknown event received {}", ev);
        case SHUTDOWN:
            mLog.d("Event received {}", ev);
            mRunning = false;
            break;
        case PEER_WELCOME: {
            int fd;
            read(mEventPipe[0], &fd, sizeof(fd));
            std::unique_lock<std::recursive_mutex> lock(mPeersMutex);
            auto it = mPeers.find(fd);
            if (it != mPeers.end()) {
                sendMsg_PeerInfo(it->second);
                sendMsg_Discovery(it->second);
            }
            break;
        }
        case PEER_CLOSE: {
            int fd;
            read(mEventPipe[0], &fd, sizeof(fd));
            mLog.d("Closing connection with {}", fd);
            removePeer(fd);
            break;
        }
        case TRY_CONNECT: {
            std::random_device random_device;
            std::mt19937 engine{random_device()};
            while (getNumClients() < kTargetNumPeers && mAddressPool.size() > 0) {
                std::uniform_int_distribution<int> dist(0, mAddressPool.size() - 1);
                int idx = dist(engine);
                aConnect(mAddressPool[idx]);
                mAddressPool[idx].swap(mAddressPool.back());
                mAddressPool.resize(mAddressPool.size()-1);
            }
            break;
        }
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
    epollCtl(EPOLL_CTL_ADD, mEventPipe[0], EPOLLIN, -2);

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
            newPeer(event);
        } else {
            // Serve this socket
            servePeer(event.data.fd);
        }
    }

    //Stop epoll & Pipes
    close(mEpollFd);
    close(mEventPipe[0]);
    close(mEventPipe[1]);

    //Stop all sockets
    {
        std::unique_lock lock(mPeersMutex);
        for(auto& i : mPeers){
            close(i.first);
        }
        mPeers.clear();
    }

    close(mMainSocket);
    mMainSocket = -1;

    mLog.t("thread stopped");
}

int P2P::getNumClients(){
    std::unique_lock lock(mPeersMutex);

    return mPeers.size();
}
