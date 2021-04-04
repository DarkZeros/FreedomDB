#pragma once

#include "common/nocopyormove.h"
#include "core/log.h"
#include "db/pool.h"

#include <string>
#include <tuple>
#include <pqxx/pqxx>

template <>
struct fmt::formatter<std::optional<pqxx::result>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }  template <typename FormatContext>
    auto format(const std::optional<pqxx::result>& p, FormatContext& ctx) {
        if (!p || p->columns() == 0)
            return format_to(ctx.out(), "Empty pqxx::result");
        return format_to(ctx.out(), "{}", *p);
    }
};

template <>
struct fmt::formatter<pqxx::result> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }  template <typename FormatContext>
    auto format(const pqxx::result& p, FormatContext& ctx) {
        std::vector<std::string> rows(p.size()+2);
        std::string sep = "";
        for(auto c=0; c<p.columns(); c++){
            std::string_view colname = p.column_name(c);
            size_t size = colname.size();
            std::vector<std::string_view> rval;
            for(auto r=0; r<p.size(); r++){
                rval.emplace_back(p[r][c].c_str());
                size = std::max(size, rval.back().size());
            }
            rows[0] += fmt::format("{}{:{}}", sep, colname, size);
            for(auto r=0; r<p.size(); r++){
                rows[r+2] += fmt::format("{}{:{}}", sep, rval[r], size);
            }
            sep = " | ";
        }
        rows[1] = fmt::format("{:->{}}", "", rows[0].size());
        return format_to(ctx.out(), "{}", fmt::join(rows, "\n"));
    }
};

class DB : private NoCopyOrMove {
public:
    // Connection pools to access postgresql as admin, or default DB
    enum ConnType {
        ADMIN,
        USER,
        SIZE
    };

    // Fix connection args
    const int mNetID;
    const std::string mAddress;
    const std::string mUser;
    const std::string mPass;
    const std::string mDbName;
    
private:
    Log mLog = Log(Log::Type::DB);
    std::array<Pool, ConnType::SIZE> mConn;

    std::string options(const std::string& user, const std::string& pass, const std::string& db = "");
    
    template <class Type, typename... Args> 
    std::optional<pqxx::result> _sql(ConnType type, const std::string& cmd, Args&&... args){
        try {
            auto c = mConn[type].get();
            mLog.t("DB:{} User:{} Host:{}", c.mPtr->dbname(), c.mPtr->username(), c.mPtr->hostname());

            if (!c.mPtr)
                return {};

            // Create a transactional (?) object.
            Type W(c);

            // Execute SQL query
            auto res = W.exec( fmt::format(cmd, W.esc(std::forward<Args>(args))...) );
            mLog.t("SQL:{}", fmt::format(cmd, W.esc(std::forward<Args>(args))...));

            // Commit it
            W.commit();

            // Return result
            return res;
        } catch (const std::exception &e) {
            mLog.e(e.what());
        }
        return {};
    }

public:
    DB(const int netID,
       const std::string& addr = "127.0.0.1:5432",
       const std::string user = "postgres",
       const std::string pass = "postgres")
        : mAddress(addr), mUser(user), mPass(pass), mNetID(netID),
          mDbName(fmt::format("freedomdb_{}", mNetID)),
          mConn{options(mUser, mPass, "postgres"),
            options(mDbName, mDbName, mDbName)} {};

    bool hasConnection();

    template <typename... Args>
    std::optional<pqxx::result> admin(Args&&... args)
    {return _sql<pqxx::nontransaction>(ConnType::ADMIN, std::forward<Args>(args)...);}
    template <typename... Args>
    std::optional<pqxx::result> noTransact(Args&&... args)
    {return _sql<pqxx::nontransaction>(ConnType::USER, std::forward<Args>(args)...);}
    template <typename... Args>
    std::optional<pqxx::result> rwTransact(Args&&... args)
    {return _sql<pqxx::work>(ConnType::USER, std::forward<Args>(args)...);}
    template <typename... Args>
    std::optional<pqxx::result> roTransact(Args&&... args)
    {return _sql<pqxx::read_transaction>(ConnType::USER, std::forward<Args>(args)...);}
    

    bool initialize();

    int getBlockNum();
};