#ifndef SLIDING_WINDOW_LIMITER_H
#define SLIDING_WINDOW_LIMITER_H

#include <unordered_map>
#include <deque>
#include <string>
#include <chrono>

class SlidingWindowLimiter {
private:
    std::unordered_map<std::string, std::deque<long long>> requestLog;

    int maxRequests;
    int windowMs;

public:
    SlidingWindowLimiter(int maxReq = 100, int window = 1000);

    bool allowRequest(const std::string& ip);

private:
    long long currentTimeMs();
};

#endif