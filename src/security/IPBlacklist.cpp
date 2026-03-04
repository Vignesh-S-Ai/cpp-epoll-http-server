#include "IPBlacklist.hpp"

using Clock = std::chrono::steady_clock;

void IPBlacklist::blockIP(const std::string& ip, int seconds)
{
    blocked_ips[ip] =
        Clock::now() + std::chrono::seconds(seconds);
}

bool IPBlacklist::isBlocked(const std::string& ip)
{
    auto it = blocked_ips.find(ip);

    if (it == blocked_ips.end())
        return false;

    if (Clock::now() > it->second) {
        blocked_ips.erase(it);
        return false;
    }

    return true;
}
