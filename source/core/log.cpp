#include <iostream>
#include <fmt/color.h>

#include "core/log.h"

std::mutex Log::mMutex;
std::atomic<bool> Log::mStdout = true;
std::atomic<bool> Log::mColored = true;
std::atomic<Log::Level> Log::mVerboseLevel = Log::kDefaultVerboseLevel;

using namespace fmt;

std::map<int, std::pair<Log::filterFunction, Log::callbackFunction>> Log::mCallbacks = {
    // Uncolored
    {0,
      {
        [](Type, Level level) {return Log::mStdout && !Log::mColored && level >= mVerboseLevel;},
        [](Type t, Level l, const std::string& str) {
          auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id()) % 0x1000000;
          fmt::print("{:4x} {:3} {:c} {}\n",
              tid,
              magic_enum::enum_name(t),
              magic_enum::enum_name(l)[0],
              str);
        }
      }
    },
    // Colored
    {1,
      {
        [](Type, Level level) {return Log::mStdout && Log::mColored && level >= mVerboseLevel;},
        [](Type t, Level l, const std::string& str) {
          auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id()) % 0x1000000;
          static auto lColors = std::vector<text_style>{
            fg(color::light_green),
            fg(color::cyan),
            fg(color::white),
            fg(color::yellow),
            fg(color::red),
          };
          static auto tColors = std::vector<text_style>{
            fg(color::red),
            fg(color::yellow),
            fg(color::cyan),
            fg(color::magenta),
            fg(color::green),
            fg(color::light_green),
          };
          auto iLevel = *magic_enum::enum_index(l);
          auto iType = *magic_enum::enum_index(t);

          fmt::print(fg(color(tid)), "{:6x}", tid);
          fmt::print(tColors[iType], " {:3}", magic_enum::enum_name(t));
          fmt::print(lColors[iLevel], " {:c}", magic_enum::enum_name(l)[0]);
          fmt::print(" {}\n", str);
        }
      }
    }
};
std::atomic<int> Log::mID = mCallbacks.size();

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
