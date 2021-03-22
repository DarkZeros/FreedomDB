#include <catch2/catch_all.hpp>
#include <process.h>
#include <fmt/core.h>
#include <chrono>

#include "p2p/p2p.h"

namespace {
    using namespace std::chrono_literals;

    constexpr auto kWaitTimeOut = 200ms;

    constexpr auto kPort1 = 12301;
    constexpr auto kPort2 = 12302;
    constexpr auto kPort3 = 12303;
};

TEST_CASE("basic functionality", "[P2P]") {
    P2P p2p;

    SECTION("initial state") {
        CHECK(p2p.isRunning() == false);
        CHECK(p2p.getNumClients() == 0);
    }
    SECTION("start server") {
        p2p.start();
        CHECK(p2p.isRunning() == true);
        CHECK(p2p.getNumClients() == 0);
    }
    SECTION("start 2 times server") {
        p2p.start();
        p2p.start();
        CHECK(p2p.isRunning() == true);
        CHECK(p2p.getNumClients() == 0);
    }
    SECTION("receive a connection and handle it, be able to change ports") {
        for(auto& port : {P2P::kDefaultListenPort, kPort1}) {
          p2p.stop();
          p2p.mListenPort = port;
          p2p.start();
          REQUIRE(p2p.isRunning() == true);

          procxx::process telnet{"telnet", "127.0.0.1", fmt::format("{}", port)};
          telnet.exec();
          std::this_thread::sleep_for(kWaitTimeOut);
          CAPTURE(port);
          CHECK(p2p.getNumClients() == 1);
          telnet.close(procxx::pipe_t::write_end());
          std::this_thread::sleep_for(kWaitTimeOut);
          CHECK(p2p.getNumClients() == 0);
        }
    }
    SECTION("stop server") {
        p2p.stop();
        CHECK(p2p.isRunning() == false);
    }
}

TEST_CASE("connect one to another", "[P2P]") {
    P2P client1, client2, client3;

    client1.mBootStrap = {}; // Do not connect, just wait
    client1.mListenPort = kPort1;
    client1.start();

    client2.mBootStrap = {fmt::format("127.0.0.1:{}", kPort1)};
    client2.mListenPort = kPort2;

    client3.mBootStrap = {fmt::format("127.0.0.1:{}", kPort1)};
    client3.mListenPort = kPort3;

    REQUIRE(client1.getNumClients() == 0);
    REQUIRE(client2.getNumClients() == 0);
    REQUIRE(client3.getNumClients() == 0);

    SECTION("connect 2 -> 1") {
        client2.start();
        std::this_thread::sleep_for(kWaitTimeOut);

        CHECK(client1.getNumClients() == 1);
        CHECK(client2.getNumClients() == 1);

        client2.stop();
        std::this_thread::sleep_for(kWaitTimeOut);

        CHECK(client1.getNumClients() == 0);
        CHECK(client2.getNumClients() == 0);
    }

    SECTION("chain connect 3 -> 1 <-> 2") {
        client2.start();
        std::this_thread::sleep_for(kWaitTimeOut);

        REQUIRE(client1.getNumClients() == 1);
        REQUIRE(client2.getNumClients() == 1);
        REQUIRE(client3.getNumClients() == 0);

        Log::e(Log::Type::DB, "STARTING CLIENT 3");
        client3.start();
        std::this_thread::sleep_for(2*kWaitTimeOut);

        CHECK(client1.getNumClients() == 2);
        CHECK(client2.getNumClients() == 2);
        CHECK(client3.getNumClients() == 2);
    }
}
