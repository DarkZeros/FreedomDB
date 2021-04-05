#define CATCH_CONFIG_RUNTIME_STATIC_REQUIRE
#include <catch2/catch_all.hpp>
#include <type_traits>

#include "common/nocopyormove.h"

TEST_CASE("no copy no move", "[Util]") {
    STATIC_REQUIRE_FALSE(std::is_move_constructible<NoCopyOrMove>::value);
    STATIC_REQUIRE_FALSE(std::is_move_assignable<NoCopyOrMove>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible<NoCopyOrMove>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable<NoCopyOrMove>::value);
}

TEST_CASE("no copy", "[Util]") {
    STATIC_REQUIRE(std::is_move_constructible<NoCopy>::value);
    STATIC_REQUIRE(std::is_move_assignable<NoCopy>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible<NoCopy>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable<NoCopy>::value);
}

TEST_CASE("no move", "[Util]") {
    STATIC_REQUIRE_FALSE(std::is_move_constructible<NoMove>::value);
    STATIC_REQUIRE_FALSE(std::is_move_assignable<NoMove>::value);
    STATIC_REQUIRE(std::is_copy_constructible<NoMove>::value);
    STATIC_REQUIRE(std::is_copy_assignable<NoMove>::value);
}