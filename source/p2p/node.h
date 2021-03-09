#pragma once

#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <netdb.h>
#include <msgpack.hpp>

/**
* A node is an stablished connection with a socket
* It holds 1 thread to attend/send/recv/events
*  Also contains important data about the client
*/
struct Node {
    // We could have multiple accesses to the Node data
    //  It is better to protect it
    std::mutex mMutex;

    sockaddr_in mSockAddr;
    std::vector<uint8_t> mRxBuf;
    msgpack::unpacker mUnpacker;

    std::string mVersion;
};
