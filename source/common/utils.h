#pragma once

#include <vector>
#include <string>
#include <string_view>

#include "core/log.h"

// Split a string to std::string // std::string_view
template <class T, class S>
std::vector<T> split(const std::string& ori, const S sep )
{
    std::vector<T> results;
    size_t pos = 0;
    while ( pos < ori.size() ) {
        auto npos = ori.find(sep, pos);
        if (npos == pos) {
            pos = npos+1;
        } else if (npos != ori.npos) {
            results.emplace_back(ori.data() + pos, npos-pos);
            pos = npos+1;
        } else {
            results.emplace_back(ori.data() + pos, ori.size() - pos);
            break;
        }
    }
    return results;
}