#include <catch2/catch_all.hpp>

#include "db/db.h"

namespace {
    const std::string kUser = "freedomdb";
    const std::string kPass = "freedomdb";
    const std::string kAddr = "127.0.0.1:5432";
    const std::string kDB = "postgres";
    const auto kOptions = fmt::format("postgresql://{}:{}@{}/{}", kUser, kPass, kAddr, kDB);
}

TEST_CASE("DB pool functionality", "[DB][Pool]") {
    Pool pool(kOptions);

    REQUIRE(pool.pool().size() == 0);

    SECTION("get from pool a connection") {
        CHECK(pool.get()->is_open() == true);
    }

    SECTION("get a new one, then store it back") {
        auto obj = pool.get();
        CHECK(pool.pool().size() == 0);
        obj.wait();
        CHECK(pool.pool().size() == 1);
    }

    SECTION("get a new one, then store it back automatically") {
        {
            auto obj = pool.get();
        }
        CHECK(pool.pool().size() == 1);
    }

    SECTION("reuse the one in the pool") {
        {
            auto obj = pool.get();
        }
        REQUIRE(pool.pool().size() == 1);
        auto ptr = pool.pool().front().get();
        auto obj = pool.get();
        CHECK(obj.mPtr.get() == ptr);
    }

    SECTION("multiple store + save") {
        {
            auto obj = pool.get();
        }
        CHECK(pool.pool().size() == 1);
        {
            auto obj = pool.get();
            auto obj2 = pool.get();
            auto obj3 = pool.get();
        }
        CHECK(pool.pool().size() == 3);
        {
            auto obj = pool.get();
            CHECK(pool.pool().size() == 2);
            auto obj2 = pool.get();
            CHECK(pool.pool().size() == 1);
            auto obj3 = pool.get();
            CHECK(pool.pool().size() == 0);
        }
    }
}