#pragma once

#include <functional>
#include <future>

// Give it a function, and it will call it lazyly, or at destruction
class Lazy
{
    std::future<void> mFuture;

public:
    Lazy(std::function<void()> f) : mFuture(std::async(std::launch::deferred, f)) {}
    ~Lazy() {mFuture.wait();}

    void wait() {mFuture.wait();}
};
