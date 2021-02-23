#include <stdarg.h>

#include "core/log.h"

std::vector<std::pair<std::string, Log::callbackFunction>> Log::mFordwards;
std::mutex Log::mMutex;
constexpr const char * Log::type2str[];
bool Log::alsoStdOut = true;
Log::Level Log::minLevel = DEFAULT_MIN_LOGLEVEL;

Log::Log() {
}
Log::Log(const std::string& id) {
    mID = id;
}
Log::Log(const std::string& id, const Log::callbackFunction& callback) {
    mID = id;
    attach(callback);
}
void Log::attach(const callbackFunction& callback){
    mAttached = true;
    add(mID, callback);
}
Log::~Log(){
    if(mAttached)
        remove(mID);
}

void Log::log(const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(NOTYPE, NOLEVEL, fmt, args);
    va_end(args);
}

void Log::log(Level level, const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(NOTYPE, level, fmt, args);
    va_end(args);
}

void Log::log(Type type, Level level, const std::string & fmt, ...){
    va_list args;
    va_start(args, fmt);
    log(type, level, fmt.c_str(), args);
    va_end(args);
}

void Log::log(Type type, Level level, const char * fmt, ...){
    if(level < minLevel) return;
    if(type >= Type::SIZE) return;

    std::string str(4096, 0);
    va_list args;
    va_start(args, fmt);
    vsnprintf(&str[0], 4090, fmt, args);
    va_end(args);

    //Send it to the log
    forward(type, level, str);

    if(alsoStdOut){
        fprintf(stdout, "%s-%c: %s\n", type2str[type], (char)level, str.c_str());
    }
}


void Log::forward(Type type, Level level, const std::string& str) {
    std::unique_lock<std::mutex> lock(mMutex);
    for(auto& i : mFordwards){
        i.second(type, level, str);
    }
}

void Log::add(const std::string& id, const callbackFunction& callback) {
    std::unique_lock<std::mutex> lock(mMutex);
    mFordwards.push_back(std::make_pair(id, callback));
}

void Log::remove(const std::string& id) {
    std::unique_lock<std::mutex> lock(mMutex);
    for(auto it = mFordwards.begin(); it != mFordwards.end();){
        if(it->first == id){
            it = mFordwards.erase(it);
        }else{
            it ++;
        }
    }
}
