#define CATCH_CONFIG_RUNTIME_STATIC_REQUIRE
#include <catch2/catch_all.hpp>
#include <type_traits>

#include "common/nocopyormove.h"

CATCH_STATIC_REQUIRE_FALSE(std::is_move_constructible<NoCopyOrMove>::value);

TEST_CASE("no copy", "[Util]") {
    NoCopyOrMove nc;

    /*SECTION("can be moved constructed") {
        NoCopyOrMove nc1(std::move(NoCopyOrMove());
    }

    SECTION("can be moved") {
        NoCopyOrMove nc1;
        nc1 = std::move(nc);
    }*/
}
