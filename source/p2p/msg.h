#pragma once

#include <msgpack.hpp>
#include <array>
#include <fmt/format.h>

namespace Msg {

enum class Type  {
    PEER_INFO,
    DISCOVERY,
    DISCONNECT,
};

struct Any {
    Type type;
    msgpack::object data;
    MSGPACK_DEFINE(type, data);
};
struct PeerInfo {
    // Irrelevant Misc information
    std::array<char, 16> mName;
    // General information
    int mListenPort;
    int mVersion;
    int mNetID;
    int mUID;
    MSGPACK_DEFINE(mName, mListenPort, mVersion, mNetID, mUID);
};
struct Discovery {
    std::vector<std::string> mAddresses;
    MSGPACK_DEFINE(mAddresses);
};
struct Disconnect {
    enum class Reason {
        UNKNOWN, // Default reason, for no reason
        WRONG_NETWORK, // Network ID missmatch
        BLOCKING, // The peer missbehaved, blocking
        ROTATING, // We have too many conections, closing some
        // Add More
    } mReason;
    std::string mText;
    MSGPACK_DEFINE(mReason, mText);
};
}; // namepsace Msg

MSGPACK_ADD_ENUM(Msg::Type);
MSGPACK_ADD_ENUM(Msg::Disconnect::Reason);

// Printers
template <>
struct fmt::formatter<Msg::PeerInfo> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }  template <typename FormatContext>
    auto format(const Msg::PeerInfo& d, FormatContext& ctx) {
        return format_to(ctx.out(), 
            "Name:{}, Addr:{}, Version:{}, NetId:{}, UID:{}",
             d.mName.data(), d.mListenPort, d.mVersion, d.mNetID, d.mUID);
    }
};
template <>
struct fmt::formatter<Msg::Discovery> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }  template <typename FormatContext>
    auto format(const Msg::Discovery& d, FormatContext& ctx) {
        return format_to(ctx.out(), "'{}'", fmt::join(d.mAddresses, ", "));
    }
};
template <>
struct fmt::formatter<Msg::Disconnect> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }  template <typename FormatContext>
    auto format(const Msg::Disconnect& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}:{}", 
            magic_enum::enum_name(d.mReason), d.mText);
    }
};
