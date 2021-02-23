#pragma once

#include <functional>
#include <vector>
#include <mutex>

#define DEFAULT_MIN_LOGLEVEL DEBUG

class Log {
public:
  enum Level{
      DEBUG = 'D',
      INFO = 'I',
      WARNING = 'W',
      ERROR = 'E',
      NOLEVEL = '?',
  };
  enum Type{
      CORE = 0,
      TIMER,
      P2P,
      NOTYPE,
      SIZE
  };
  typedef std::function<void(Type, Level, const std::string &)> callbackFunction;

private:
    static std::vector<std::pair<std::string, callbackFunction>> mFordwards;
    static std::mutex mMutex;
    static void forward(Type type, Level level, const std::string& str);

    std::string mID;
    bool mAttached = false;
public:
    static constexpr const char * type2str[] = {"Core", "Timer", "P2P", ""};
    static bool alsoStdOut;
    static Level minLevel;

    static void log(Type type, Level level, const std::string & fmt, ...);
    static void log(Type type, Level level, const char * fmt, ...);
    static void log(Level level, const char * fmt, ...);
    static void log(const char * fmt, ...);
    static void add(const std::string& id, const callbackFunction& callback);
    static void remove(const std::string& id);

    Log();
    Log(const std::string& id);
    Log(const std::string& id, const callbackFunction& callback);
    ~Log();
    void attach(const callbackFunction& callback);
};
