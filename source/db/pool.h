#pragma once

#include "common/nocopyormove.h"
#include "common/lazy.h"
#include "core/log.h"

#include <optional>
#include <mutex>
#include <string>
#include <pqxx/pqxx>

class Pool : public NoCopyOrMove {
    Log mLog = Log(Log::Type::DB);

    std::mutex mMutex;
    std::list<std::unique_ptr<pqxx::connection>> mPool;
    std::string mOptions;

    pqxx::connection build();

public:
    struct Element {
        std::unique_ptr<pqxx::connection> mPtr;
        Lazy mLazyDtr;

        pqxx::connection* operator->() {return mPtr.get();}
        pqxx::connection* operator*() {return mPtr.get();}
        operator pqxx::connection&() {return *mPtr.get();}
        operator Lazy&() {return mLazyDtr;}
        void wait() {return mLazyDtr.wait();}

        Element(std::unique_ptr<pqxx::connection>& ptr, Pool& pool);
    };

    friend fmt::formatter<Pool>;
    Pool(const std::string& options) : mOptions(options){};

    Pool::Element get();
    void put(std::unique_ptr<pqxx::connection>& mPtr);
    const auto& pool(){return mPool;}
};
template <>
struct fmt::formatter<Pool> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }  template <typename FormatContext>
    auto format(const Pool& p, FormatContext& ctx) {
        std::vector<std::string> str;
        for(auto& i : p.mPool){
            str.emplace_back(fmt::format("{}", fmt::ptr(i)));
        }
        return format_to(ctx.out(),
            "DBPool:{}, [{}]", p.mOptions, fmt::join(str, ", "));
    }
};