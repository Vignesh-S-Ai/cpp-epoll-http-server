#include "sliding_window_limiter.h"

SlidingWindowLimiter::SlidingWindowLimiter(int maxReq, int window)
    : maxRequests(maxReq), windowMs(window) {}

#include <iostream>

long long SlidingWindowLimiter::currentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

bool SlidingWindowLimiter::allowRequest(const std::string& ip) {

    long long now = currentTimeMs();
    auto &dq = requestLog[ip];

    while (!dq.empty() && now - dq.front() > windowMs) {
        dq.pop_front();
    }

    if (dq.size() >= maxRequests) {
        std::cout << "Rate limit exceeded for IP: " << ip << std::endl;
        return false;
    }

    dq.push_back(now);
    return true;
}