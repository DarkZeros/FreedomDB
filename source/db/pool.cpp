
#include "db/pool.h"

#include <fmt/format.h>

Pool::Element::Element(std::unique_ptr<pqxx::connection>& ptr, Pool& pool)
    : mPtr(std::move(ptr)),
      mLazyDtr(std::bind([&](Element* e){pool.put(e->mPtr);}, this)){};

Pool::Element Pool::get(){
    std::unique_lock<std::mutex> lock(mMutex);
    std::unique_ptr<pqxx::connection> c;
    if (mPool.size()) {
        c = std::move(mPool.front());
        mPool.pop_front();
        mLog.t("Get from Pool {}", fmt::ptr(c.get()));
    } else {
        // Build it
        try {
            c = std::make_unique<pqxx::connection>(mOptions);
            mLog.t("Get new one {}", fmt::ptr(c.get()));
        } catch (const std::exception &e) {
            mLog.e(e.what());
            mLog.e("Failed with options {}", mOptions);
        }
    }
    mLog.t(*this);

    return Element(c, *this);
}
void Pool::put(std::unique_ptr<pqxx::connection>& mPtr){
    std::unique_lock<std::mutex> lock(mMutex);
    if (mPtr.get() == nullptr)
        return;
    mLog.t("Put ptr called {}", fmt::ptr(mPtr.get()));
    mPool.emplace_back(std::move(mPtr));
    mLog.t(*this);
}