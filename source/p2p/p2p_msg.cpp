#include <stdio.h>
#include <string.h> // memset()
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <fmt/format.h>

#include "p2p/p2p.h"
#include "p2p/msg.h"
#include "core/log.h"

void P2P::decodeMsg(Peer& peer, const msgpack::object& obj){
    Msg::Any any = obj.convert();
    auto& data = any.data;
    auto name = magic_enum::enum_name(any.type);

    #define DECODE(ty, o, f) \
        case ty: { o m = data.convert(); \
            mLog.t("{} msg recv {}", name, m); \
            f(peer, m); \
        }; break;

    switch(any.type){
        DECODE(Msg::Type::PEER_INFO, Msg::PeerInfo, decodeMsg_PeerInfo);
        DECODE(Msg::Type::DISCOVERY, Msg::Discovery, decodeMsg_Discovery);
        DECODE(Msg::Type::DISCONNECT, Msg::Disconnect, decodeMsg_Disconnect);
    }
}
void P2P::decodeMsg_Discovery(Peer& peer, const Msg::Discovery& msg){
    // Add addresses to the pool
    for (auto& addr : msg.mAddresses) {
        mAddressPool.emplace_back(addr);
    }
    sendThreadEvent(TRY_CONNECT);
}

void P2P::decodeMsg_Disconnect(Peer& peer, const Msg::Disconnect& msg){
    // Add addresses to the pool
    mLog.e("{} disconnected due to {}", peer, msg);
    removePeer(peer.mFd);
}

void P2P::decodeMsg_PeerInfo(Peer& peer, const Msg::PeerInfo& msg){
    // Inmediately remove the peer, do not let more msg from coming from it
    // Check the peer is connected on the same network
    if (msg.mNetID != mOwnPeerInfo.mNetID) {
        mLog.w("Client not of the same network {} != {}", msg.mNetID, mOwnPeerInfo.mNetID);
        // TODO: Send reason for disconnect
        sendMsg_Disconnect(peer, Msg::Disconnect::Reason::WRONG_NETWORK);
        removePeer(peer.mFd);
    }
    else if (msg.mUID == mOwnPeerInfo.mUID) {
        mLog.w("Peer {} is likely ourselve, exit {}", peer, msg.mUID);
        removePeer(peer.mFd);
    } else {
        // The peer info is valid, set it
        peer.mName = msg.mName;
        peer.mListenPort = msg.mListenPort;
        peer.mVersion = msg.mVersion;
        peer.mNetID = msg.mNetID;
        peer.mUID = msg.mUID;

        // Then send our welcome pack as well (PEER_INFO + DISCOVERY)
        if (!peer.mReady && peer.mDirection == Peer::Direction::IN) {
            peer.mReady = true;
            sendThreadEvent(PEER_WELCOME);
            sendThreadEventData(peer.mFd);
        }
    }
}

void P2P::sendMsg(Peer& peer, const msgpack::object& obj){
    std::unique_lock<std::recursive_mutex> lock(peer.mMutex);
    msgpack::sbuffer packed;
    msgpack::pack(&packed, obj);
    if (packed.size() != write(peer.mFd, packed.data(), packed.size())) {
        mLog.e("Error sending to socket on {}", peer);
    }
}

void P2P::sendMsg_PeerInfo(Peer& peer){
    msgpack::zone z;
    auto msg = msgpack::object(Msg::Any { Msg::Type::PEER_INFO, 
        msgpack::object(Msg::PeerInfo {mOwnPeerInfo}, z) }, z);
    mLog.t("Sending {}", Msg::PeerInfo {mOwnPeerInfo});
    sendMsg(peer, msg);
}
void P2P::sendMsg_Disconnect(Peer& peer, const Msg::Disconnect::Reason& r, const std::string& text){
    msgpack::zone z;
    auto msg = msgpack::object(Msg::Any { Msg::Type::DISCONNECT, 
        msgpack::object(Msg::Disconnect {r, text}, z) }, z);
    mLog.t("Sending {}", Msg::Disconnect {r, text});
    sendMsg(peer, msg);
}
void P2P::sendMsg_Discovery(Peer& peer){
    msgpack::zone z;
    // Aggregate all adddresses and send them
    std::unique_lock<std::recursive_mutex> lock(mPeersMutex);
    std::vector<std::string> addr;
    for(auto& [fd, p] : mPeers) {
        if (p.mReady)
            addr.emplace_back(fmt::format("{}:{}", p.mConAddress, p.mListenPort));
    }
    if (addr.size() == 0)
        return;
    auto msg = msgpack::object(Msg::Any { Msg::Type::DISCOVERY, 
        msgpack::object(Msg::Discovery {addr}, z) }, z);
    mLog.t("Sending {}", Msg::Discovery {addr});
    sendMsg(peer, msg);
}