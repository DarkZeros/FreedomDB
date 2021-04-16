#include <catch2/catch_all.hpp>

#include "common/utils.h"

TEST_CASE("Split string to strings", "[Utils]") {
    auto [input, delimit, expected] = GENERATE(table<std::string, char, std::string>({
        {"hi\nhow\ngood", '\n', "hi:how:good"},
        {"\nhi\n\nhow\ngood\n", '\n', "hi:how:good"},
        {"\nhi\nhow\n\n\n\ngood\n", '\n', "hi:how:good"},
        {"\nhi\nhow\n\n\n\ngood\n", 'h', "\n:i\n:ow\n\n\n\ngood\n"},
        {"\nhi\nhow\n\n\n\ngood\n", 'o', "\nhi\nh:w\n\n\n\ng:d\n"},
    }));

    SECTION("split to strings") {
        auto s = split<std::string>(input, delimit);
        std::string str = fmt::format("{}", fmt::join(s, ":"));
        CAPTURE(input, delimit, expected);
        CHECK(str == expected);
    }
    SECTION("split to string_views") {
        auto s = split<std::string_view>(input, delimit);
        std::string str = fmt::format("{}", fmt::join(s, ":"));
        CAPTURE(input, delimit, expected);
        CHECK(str == expected);
    }
}