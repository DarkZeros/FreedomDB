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
        bool mHandshake = false;
    };
private:
    const std::string mPort = "11250";
    static constexpr int mListenQueue = 10;

    int mMainSocket = -1;
    int mEventFd = -1;
    std::map<int, ClientData> mClientSocket;

    std::atomic<bool> mRunning;
    std::thread mThread;
    std::mutex mThreadMutex, mDataMutex;
public:
    P2P();
    ~P2P();

    bool start();
    void stop();

    int thread_loop(void);

    int getNumClients();
};
