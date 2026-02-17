#pragma once
#include <string>
#include <mutex>

enum class LogLevel {
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static void init(const std::string& filename);
    static void log(LogLevel level, const std::string& message);

private:
    static std::string levelToString(LogLevel level);
    static std::mutex log_mutex;
    static std::string log_file;
};
