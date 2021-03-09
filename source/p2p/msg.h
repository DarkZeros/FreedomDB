#pragma once

#include <msgpack.hpp>
#include <array>

namespace Msg {

enum class Type  {
    PEER_INFO,
    WELCOME,
    DISCOVERY,
};

struct Any {
    Type type;
    msgpack::object data;
    MSGPACK_DEFINE(type, data);
};
struct PeerInfo {
    int mVersion;
    //int mAddress;
    std::array<char, 16> mName;
    MSGPACK_DEFINE(mVersion, mName);
};
struct Welcome {
    PeerInfo mInfo;
    MSGPACK_DEFINE(mInfo);
};
struct Discovery {
    std::vector<std::string> mAddresses;
    MSGPACK_DEFINE(mAddresses);
};

}; // namepsace Msg

MSGPACK_ADD_ENUM(Msg::Type);
