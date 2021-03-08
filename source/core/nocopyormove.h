#pragma once

class NoCopyOrMove
{
public:
    NoCopyOrMove() = default;

    NoCopyOrMove(const NoCopyOrMove&) = delete;
    NoCopyOrMove& operator=(const NoCopyOrMove&) = delete;

    NoCopyOrMove(NoCopyOrMove&&) = delete;
    NoCopyOrMove& operator=(NoCopyOrMove&&) = delete;
};
