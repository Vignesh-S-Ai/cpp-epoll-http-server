#include "ThreatDetector.hpp"

bool ThreatDetector::isSuspicious(const std::string& ip, IPTracker& tracker)
{
    auto rate = tracker.getCurrentRate(ip);

    if (rate > RATE_THRESHOLD)
        return true;

    return false;
}
