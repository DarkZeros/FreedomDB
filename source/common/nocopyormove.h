#pragma once

class NoCopy
{
public:
    NoCopy() = default;

    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;

    NoCopy(NoCopy&&) = default;
    NoCopy& operator=(NoCopy&&) = default;
};

class NoMove
{
public:
    NoMove() = default;

    NoMove(const NoMove&) = default;
    NoMove& operator=(const NoMove&) = default;

    NoMove(const NoMove&&) = delete;
    NoMove& operator=(const NoMove&&) = delete;
};

class NoCopyOrMove : private NoCopy, NoMove {};