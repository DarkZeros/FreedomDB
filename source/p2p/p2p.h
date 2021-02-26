#pragma once

#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <netdb.h>

class P2P {
public:
    struct ClientData {
        sockaddr_in addr;
        std::vector<uint8_t> rxBuf;

        //
        bool mHandshake = false;
    };

    static constexpr auto kListenQueueLen = 10;
    static constexpr auto kDefaultListenPort = 11250;
    static const std::vector<std::string> kBootStrap;

    int mListenPort = 11250;
    std::vector<std::string> mBootStrap = kBootStrap;

private:
    int mMainSocket = -1;
    int mEventFd = -1;
    std::map<int, ClientData> mClientSocket;

    std::atomic<bool> mRunning = false;
    std::thread mThread;
    std::mutex mThreadMutex, mDataMutex;

    int listenThreadLoop(void);
    int sendThreadLoop(void);

public:
    ~P2P() {stop();}

    bool start();
    void stop();

    bool isRunning() {return mRunning;};
    int getNumClients();
};
