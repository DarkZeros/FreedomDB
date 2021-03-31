#pragma once

#include <atomic>
#include <map>
#include <mutex>

#include <magic_enum.hpp>
#include <fmt/core.h>

#include "core/lazy.h"

/**
* The log class is a global singleton mutex protected
* But you can instantiate one for easy call
* a given Type of Log
*/

class Log {
public:
    enum class Level {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
    };
    enum class Type {
        NOTYPE = 0,
        CORE,
        DB,
        TIMER,
        CMD,
        P2P,
    };

    static const Level kDefaultVerboseLevel = Level::INFO;
    typedef std::function<void(Type, Level, const std::string &)> callbackFunction;
    typedef std::function<bool(Type, Level)> filterFunction;

private:
    static std::map<int, std::pair<filterFunction, callbackFunction>> mCallbacks;
    static std::mutex mMutex;
    static std::atomic<int> mID;
public:
    // There is a special print to STDOUT with its own Level
    // It is handled by a callback[0], but it refers to these switches
    static std::atomic<bool> mStdout;
    static std::atomic<bool> mColored;
    static std::atomic<Level> mVerboseLevel;

    static const auto& getCallbacks() {return mCallbacks;}
    static Lazy add(callbackFunction callback, filterFunction filter = [](Type, Level){return true;});

    // Templates and wrappers
    template <typename S, typename... Args>
    static void log(Type type, Level level, const S& s, Args&&... args)
    {
        // Go through all Callbacks and if we have any to call
        //  then lazy evaluate the format and call it
        std::string str;
        bool done = false;
        std::unique_lock<std::mutex> lock(mMutex);
        for (auto& [id, callback] : mCallbacks) {
            auto& [filter, call] = callback;
            if (filter(type, level)) {
                if (!done)
                    str = fmt::format(s, std::forward<Args>(args)...);
                done = true;
                call(type, level, str);
            }
        }
    }
    template <typename S, typename... Args>
    static void t(Type type, const S& s, Args&&... args)
    {
        log(type, Level::TRACE, s, std::forward<Args>(args)...);
    }
    template <typename S, typename... Args>
    static void d(Type type, const S& s, Args&&... args)
    {
        log(type, Level::DEBUG, s, std::forward<Args>(args)...);
    }
    template <typename S, typename... Args>
    static void i(Type type, const S& s, Args&&... args)
    {
        log(type, Level::INFO, s, std::forward<Args>(args)...);
    }
    template <typename S, typename... Args>
    static void w(Type type, const S& s, Args&&... args)
    {
        log(type, Level::WARN, s, std::forward<Args>(args)...);
    }
    template <typename S, typename... Args>
    static void e(Type type, const S& s, Args&&... args)
    {
        log(type, Level::ERROR, s, std::forward<Args>(args)...);
    }

    // Same but when you instantiate an object, you use the object defaults
    Log(Type type);
    Type mType;

    template <typename S, typename... Args>
    void t(const S& s, Args&&... args)
    {
        log(mType, Level::TRACE, s, std::forward<Args>(args)...);
    }
    template <typename S, typename... Args>
    void d(const S& s, Args&&... args)
    {
        log(mType, Level::DEBUG, s, std::forward<Args>(args)...);
    }
    template <typename S, typename... Args>
    void i(const S& s, Args&&... args)
    {
        log(mType, Level::INFO, s, std::forward<Args>(args)...);
    }
    template <typename S, typename... Args>
    void w(const S& s, Args&&... args)
    {
        log(mType, Level::WARN, s, std::forward<Args>(args)...);
    }
    template <typename S, typename... Args>
    void e(const S& s, Args&&... args)
    {
        log(mType, Level::ERROR, s, std::forward<Args>(args)...);
    }
};
