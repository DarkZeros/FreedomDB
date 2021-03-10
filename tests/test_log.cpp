#include <catch2/catch_all.hpp>
#include <fmt/core.h>
#include <fmt/format.h>

#include "core/log.h"

TEST_CASE("basic log functionality", "[Log]") {
    Log::mStdout = false;

    std::vector<std::tuple<Log::Type, Log::Level, std::string>> output;
    auto saveAll = [&](Log::Type t, Log::Level l, const std::string& str){
        output.emplace_back(t, l, str);
    };

    SECTION("add and remove callbacks") {
        CHECK(output.size() == 0);

        auto c1 = Log::add(saveAll);
        Log::e(Log::Type::P2P, "asd");

        CHECK(output.size() == 1);

        auto c2 = Log::add(saveAll);
        Log::e(Log::Type::P2P, "asd");

        CHECK(output.size() == 3);

        c1.wait();
        Log::e(Log::Type::P2P, "asd");

        CHECK(output.size() == 4);

        c2.wait();
        Log::e(Log::Type::P2P, "asd");

        CHECK(output.size() == 4);
    }

    SECTION("Log objects preserve type") {
        CHECK(output.size() == 0);

        Log log(Log::Type::DB);
        auto c = Log::add(saveAll);
        log.e("asd");

        REQUIRE(output.size() == 1);
        CHECK(std::get<0>(output[0]) == Log::Type::DB);
    }
}

TEST_CASE("benchmark log functionality", "[.][Log]") {
    Log::mStdout = false;

    std::vector<std::tuple<Log::Type, Log::Level, std::string>> output;
    auto saveAll = [&](Log::Type t, Log::Level l, const std::string& str){
        output.emplace_back(t, l, str);
    };
    std::vector<int> costlyVector(10000, 0);

    BENCHMARK("empty call cost"){
        Log::e(Log::Type::P2P, "asd");
    };

    BENCHMARK("add remove callback"){
        auto c = Log::add(saveAll);
        c.wait();
    };

    BENCHMARK("empty call with costly argument"){
        Log::e(Log::Type::P2P, "{}", fmt::join(costlyVector, ","));
    };
    BENCHMARK("add/remove + call with costly argument"){
        auto c = Log::add(saveAll);
        Log::e(Log::Type::P2P, "{}", fmt::join(costlyVector, ","));
    };
    BENCHMARK("add/remove + call with cheap argument"){
        auto c = Log::add(saveAll);
        Log::e(Log::Type::P2P, "");
    };
}
