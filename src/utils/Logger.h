#pragma once
#include <string>
#include <iostream>
#include <sstream>

enum class LogLevel { DEBUG, INFO, WARN, ERROR, NONE };

class Logger {
public:
    static Logger& get() {
        static Logger instance;
        return instance;
    }

    void set_level(LogLevel level) {
        m_level = level;
    }

    bool want_log(LogLevel level) {
        return (m_level <= level);
    }

    template<typename... Args>
    void debug(const Args&... args) {
        if (m_level <= LogLevel::DEBUG) log("[DEBUG]", args...);
    }

    template<typename... Args>
    void info(const Args&... args) {
        if (m_level <= LogLevel::INFO) log("[INFO ]", args...);
    }

    template<typename... Args>
    void warn(const Args&... args) {
        if (m_level <= LogLevel::WARN) log("[WARN ]", args...);
    }

    template<typename... Args>
    void error(const Args&... args) {
        if (m_level <= LogLevel::ERROR) log("[ERROR]", args...);
    }

private:
    Logger() = default;
    LogLevel m_level = LogLevel::INFO; // Default log level
    
    // --- C++14 COMPATIBLE RECURSIVE LOGGING ---
    template<typename T, typename... Args>
    void build_log_stream(std::ostringstream& oss, const T& first, const Args&... rest) {
        oss << first;
        build_log_stream(oss, rest...);
    }

    // Base case for the recursion
    void build_log_stream(std::ostringstream& oss) {
        // Does nothing, ends the recursion.
    }

    template<typename... Args>
    void log(const std::string& prefix, const Args&... args) {
        std::ostringstream oss;
        oss << prefix << " ";
        build_log_stream(oss, args...);
        std::cout << oss.str() << std::endl;
    }

public:
    // Prevent copying and assignment
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;
};

// Global accessor for convenience
#define LOG_DEBUG(...) Logger::get().debug(__VA_ARGS__)
#define LOG_INFO(...)  Logger::get().info(__VA_ARGS__)
#define LOG_WARN(...)  Logger::get().warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::get().error(__VA_ARGS__)

#define WANT_DEBUG (Logger::get().want_log(LogLevel::DEBUG))
#define WANT_INFO  (Logger::get().want_log(LogLevel::INFO))
#define WANT_WARN  (Logger::get().want_log(LogLevel::WARN))
#define WANT_ERROR (Logger::get().want_log(LogLevel::ERROR))
