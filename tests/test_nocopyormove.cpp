#include <catch2/catch_all.hpp>

#include "core/nocopyormove.h"

TEST_CASE("no copy", "[Util]") {
    NoCopyOrMove nc;

    /*SECTION("can be moved constructed") {
        NoCopyOrMove nc1(std::move(nc));
    }

    SECTION("can be moved") {
        NoCopyOrMove nc1;
        nc1 = std::move(nc);
    }*/
}
