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

#include "core/log.h"
#include "core/nocopyormove.h"
#include "node.h"

class P2P : private NoCopyOrMove {
public:
    enum Event : uint8_t {
        SHUTDOWN = 0, // When is closing
        CLOSE_CONNECTION,
        DATA_RECV,
        NEW_BLOCK,
        NEW_TX,
        // ...
    };

    // Callbacks can be added any time (with mutex hold)
    typedef std::function<bool(Event)> Callback;
    std::list<Callback> mCallbacks;

    static constexpr auto kListenQueueLen = 10;
    static constexpr auto kDefaultListenPort = 11250;
    static constexpr auto kUpackBuffer = 4096;
    static const std::vector<std::string> kBootStrap;

    int mListenPort = 11250;
    std::vector<std::string> mBootStrap = kBootStrap;

    std::mutex mClientMutex;
    std::map<int, Node> mClients;

private:
    Log mLog;

    int mMainSocket = -1;
    int mEventFd = -1;
    int mEpollFd = -1;

    std::mutex mTasksMutex;
    std::list<std::future<int>> mTasks;

    std::atomic<bool> mRunning = false;
    std::thread mThread;
    std::mutex mThreadMutex;

    void threadLoop(void);

    // Private functions called by thread loop
    void serveClient(int fd);
    void newClient(struct epoll_event& ev);
    void handleEvents();
    void epollCtl(int op, int fd, uint32_t ev, int data);

public:
    P2P() : mLog(Log::Type::P2P) {}
    ~P2P() {stop();}

    bool start();
    void stop();

    void aConnect(const std::string& address);
    int connect(std::string address);
    bool isRunning() {return mRunning;};
    int getNumClients();
};
