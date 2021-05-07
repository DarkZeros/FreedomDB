#include <catch2/catch_all.hpp>

#include "db/db.h"

namespace {
    const auto kNetID = 0; // Use this ID for unit testing only
}

TEST_CASE("wrong connection does not connect", "[DB]") {
    DB db(kNetID, "127.0.0.1:123", "notuser", "nopassword");
    CHECK(db.hasConnection() == false);
}
    
TEST_CASE("basic Database functionality", "[DB]") {
    DB db(kNetID);
    REQUIRE(db.hasConnection() == true);

/*
    SECTION("clean the DB, should not exist"){
        db.sql("CREATE DATABASE asd;");
        //CHECK();
        Log::e(Log::Type::DB, db.sql("SELECT datname FROM pg_database WHERE datistemplate = false;"));
        db.sql("DROP DATABASE asd;");
    }

    SECTION("we can run admin SQL commands") {
        CHECK(db.initialize() == true);
    }

    SECTION("we can connect to the DB") {

    }

    SECTION("read only SQL") {
        db.transact("SELECT * in *;");
    }
*/
}

TEST_CASE("initialize DB", "[DB]") {
    DB db(kNetID);
    REQUIRE(db.hasConnection() == true);

    db.initialize();

    Log::e(Log::Type::DB, db.noTransact(
        "SELECT table_name FROM information_schema.tables WHERE table_schema='public' AND table_type='BASE TABLE';"));
    CHECK(db.getBlockNum() == 0);
}