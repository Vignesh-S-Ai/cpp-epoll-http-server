#pragma once

#include <string>
#include "../ip_tracking/IPTracker.hpp"

class ThreatDetector {
public:
    bool isSuspicious(const std::string& ip, IPTracker& tracker);

private:
    const int RATE_THRESHOLD = 100;
};
