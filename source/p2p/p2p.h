#pragma once

#include <vector>
#include <list>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <netdb.h>
#include <future>
#include <functional>

#include "common/nocopyormove.h"
#include "core/log.h"
#include "peer.h"

class P2P : private NoCopyOrMove {
public:
    // Thread loop events
    enum Event : uint8_t {
        SHUTDOWN = 0, // When is closing
        PEER_WELCOME, // When the thread needs to greet a new peer
        PEER_CLOSE, // When we have to close a connection with a peer
        TRY_CONNECT, // When we have new addresses and might want to connect

        NEW_BLOCK,
        NEW_TX,
        // ...
    };

    // Callbacks can be added any time (with mutex hold)
    typedef std::function<bool(Event)> Callback;
    std::list<Callback> mCallbacks;

    void sendCallback(Event);

    static constexpr auto kTargetNumPeers = 20;
    static constexpr auto kListenQueueLen = 10;
    static constexpr auto kDefaultListenPort = 11250;
    static constexpr auto kUpackBuffer = 4096;
    static const std::vector<std::string> kBootStrap;

    int mListenPort = 11250;
    std::vector<std::string> mBootStrap = kBootStrap;
    std::vector<std::string> mAddressPool; //Mutex?

    std::recursive_mutex mPeersMutex;
    std::map<int, Peer> mPeers;
    Msg::PeerInfo mOwnPeerInfo = {};

private:
    static std::atomic<int> mUID;
    Log mLog = Log(Log::Type::P2P);

    int mMainSocket = -1;
    int mEventPipe[2] = {};
    int mEpollFd = -1;

    std::mutex mTasksMutex;
    std::list<std::future<int>> mTasks;

    std::atomic<bool> mRunning = false;
    std::thread mThread;
    std::mutex mThreadMutex;

    void threadLoop(void);

    // Private functions called by thread loop
    void servePeer(int fd);
    Peer& insertPeer(const sockaddr_in& sockaddr, int sock, const Peer::Direction& dir);
    void removePeer(int fd);
    void removePeer(const Peer& peer);
    void newPeer(struct epoll_event& ev);

    void handleEvents();
    void epollCtl(int op, int fd, uint32_t ev, int data);
    void sendThreadEvent(const Event& ev);
    template<class T>
    void sendThreadEventData(const T& data){
        write(mEventPipe[1], &data, sizeof(data));
    }

    void decodeMsg(Peer& peer, const msgpack::object& obj);
    void decodeMsg_Discovery(Peer& peer, const Msg::Discovery& msg);
    void decodeMsg_PeerInfo(Peer& peer, const Msg::PeerInfo& msg);
    void decodeMsg_Disconnect(Peer& peer, const Msg::Disconnect& msg);

    void sendMsg(Peer& peer, const msgpack::object& obj);
    void sendMsg_PeerInfo(Peer& peer);
    void sendMsg_Discovery(Peer& peer);
    void sendMsg_Disconnect(Peer& peer, const Msg::Disconnect::Reason& r=Msg::Disconnect::Reason::UNKNOWN, const std::string& text = {});

public:
    P2P() = default;
    ~P2P() {stop();}

    bool start();
    void stop();

    void aConnect(const std::string& address);
    int connect(const std::string& address);
    bool isRunning() {return mRunning;};
    int getNumClients();
};
