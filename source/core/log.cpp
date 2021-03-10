#include <iostream>

#include "core/log.h"

std::mutex Log::mMutex;
std::atomic<bool> Log::mStdout = true;
std::atomic<Log::Level> Log::mVerboseLevel = Log::kDefaultVerboseLevel;
std::atomic<int> Log::mID = 1;

std::map<int, std::pair<Log::filterFunction, Log::callbackFunction>> Log::mCallbacks = {
    {0,
      {
        [](Type, Level level) {return Log::mStdout && level >= mVerboseLevel;},
        [](Type t, Level l, const std::string& str) {
          fmt::print("{} {:c} {}\n", magic_enum::enum_name(t), magic_enum::enum_name(l)[0], str);
        }
      }
    }
};

Log::Log(Type type) {
    mType = type;
}

Lazy Log::add(callbackFunction callback, filterFunction filter) {
    std::unique_lock<std::mutex> lock(mMutex);
    int id = ++mID;
    mCallbacks[id] = {filter, callback};
    return Lazy([&,id](){
        std::unique_lock<std::mutex> lock(mMutex);
        auto it = mCallbacks.find(id);
        if (it != mCallbacks.end()) {
            mCallbacks.erase(it);
        }
    });
}
