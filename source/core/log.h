#pragma once

#include <atomic>
#include <map>
#include <mutex>

#include <magic_enum.hpp>
#include <fmt/core.h>
#include <fmt/format.h>

#include "common/lazy.h"

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
        UNKW = 0,
        CORE,
        DB,
        TIME,
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
    template <typename... Args>
    static void log(Level level, Type type, Args&&... args)
    {
        log(type, level, std::forward<Args>(args)...);
    }
    template <typename T>
    static std::string _format(T&& arg)
    {
        return fmt::format("{}", std::forward<T>(arg));
    }
    template <typename... Args>
    static std::string _format(Args&&... args)
    {
        return fmt::format(std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void log(Type type, Level level, Args&&... args)
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
                    str = _format(std::forward<Args>(args)...);
                done = true;
                call(type, level, str);
            }
        }
    }

    template <typename... Args>
    static void t(Type type, Args&&... args)
    {
        log(type, Level::TRACE, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void d(Type type, Args&&... args)
    {
        log(type, Level::DEBUG, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void i(Type type, Args&&... args)
    {
        log(type, Level::INFO, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void w(Type type, Args&&... args)
    {
        log(type, Level::WARN, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void e(Type type, Args&&... args)
    {
        log(type, Level::ERROR, std::forward<Args>(args)...);
    }

    // Same but when you instantiate an object, you use the object defaults
    Log(Type type);
    Type mType;

    template <typename... Args>
    void t(Args&&... args)
    {
        log(mType, Level::TRACE, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void d(Args&&... args)
    {
        log(mType, Level::DEBUG, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void i(Args&&... args)
    {
        log(mType, Level::INFO, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void w(Args&&... args)
    {
        log(mType, Level::WARN, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void e(Args&&... args)
    {
        log(mType, Level::ERROR, std::forward<Args>(args)...);
    }
};
