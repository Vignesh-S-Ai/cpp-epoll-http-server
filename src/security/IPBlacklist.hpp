#pragma once

#include <unordered_map>
#include <string>
#include <chrono>

class IPBlacklist {

public:
    void blockIP(const std::string& ip, int seconds);
    bool isBlocked(const std::string& ip);

private:

    std::unordered_map<
        std::string,
        std::chrono::steady_clock::time_point
    > blocked_ips;
};
