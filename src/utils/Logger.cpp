#include "Logger.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <thread>

std::mutex Logger::log_mutex;
std::string Logger::log_file;

void Logger::init(const std::string& filename) {
    log_file = filename;
}

std::string Logger::levelToString(LogLevel level) {
    switch(level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    localtime_r(&time_t_now, &tm);

    std::ofstream file(log_file, std::ios::app);

    file << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
         << "[" << levelToString(level) << "] "
         << "[Thread-" << std::this_thread::get_id() << "] "
         << message << "\n";

    file.close();
}
