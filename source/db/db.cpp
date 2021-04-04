
#include "db.h"

#include <vector>
#include <fmt/format.h>

std::string DB::options(const std::string& user, const std::string& pass, const std::string& db){
    //postgresql://[user[:password]@][host][:port][,...][/dbname][?param1=value1&...]
    return fmt::format("postgresql://{}:{}@{}/{}",
        user, pass, mAddress, db);
}

bool DB::hasConnection() {
    auto con = mConn[ADMIN].get();
    return con.mPtr && con->is_open();
}

bool DB::initialize() {
    // Drop Database and role
    admin("DROP DATABASE IF EXISTS {};", mDbName);
    admin("DROP ROLE {};", mDbName);
    
    //Create role and DB
    admin("CREATE ROLE {} CREATEDB CREATEROLE LOGIN PASSWORD '{}';", mDbName, mDbName);
    admin("CREATE DATABASE {} ENCODING = 'UTF-8' OWNER {}", mDbName, mDbName);

    // Create the empty blockchain table & account table & global table
    rwTransact(R"!(CREATE TABLE blockchain (
        block_id serial PRIMARY KEY,
        dbhash bytea NOT NULL,
        txmerkle bytea NOT NULL,
        nonce BIGINT NOT NULL
    );)!");
    rwTransact(R"!(CREATE TABLE account (
        acc_id serial PRIMARY KEY,
        pubkey bytea NOT NULL,
        balance BIGINT NOT NULL
    );)!");


    // Add data to the tables

/*
    // Apply TXs
    // Add final transaction to add funds / run update rule
    // Hash DB -> "dbhash"
    // Insert a final line in DB with dbhash/txmerkle/nonce -> score
*/

    return false;
}

int DB::getBlockNum(){
    auto r = roTransact("SELECT block_id FROM blockchain ORDER BY block_id DESC LIMIT 1");
    return r && r->size() ? (*r)[0][0].as<int>() : -1;
}