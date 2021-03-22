#pragma once

#include <mutex>
#include <atomic>
#include <netdb.h>

#include "p2p/msg.h"

/**
*  A peer is an stablished connection with a socket
*  Also contains important data about the client by extending the 
*  Msg::PeerInfo class
*/
struct Peer : Msg::PeerInfo {
    // We could have multiple accesses to the Peer data
    //  It is better to protect it
    std::recursive_mutex mMutex;

    // Connection related data
    int mFd;
    msgpack::unpacker mUnpacker;
    std::string mConAddress;
    int mConPort;
    bool mReady = false;
    enum class Direction {
        OUT, IN, UNKNOWN
    } mDirection = Direction::UNKNOWN;
};

template <>
struct fmt::formatter<Peer> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }  template <typename FormatContext>
    auto format(const Peer& d, FormatContext& ctx) {
        return format_to(ctx.out(), 
            "Peer: Fd{}, {}:{}, Ready:{}", d.mFd, d.mConAddress, d.mConPort, d.mReady);
    }
};
