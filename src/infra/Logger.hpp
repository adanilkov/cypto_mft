#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace crypto_hft {

class Logger {
public:
    static Logger& getInstance();

    // Initialize logger
    void initialize(const std::filesystem::path& logDir,
                   const std::string& logLevel = "info");
    
    // Logging methods
    template<typename... Args>
    void trace(const std::string& fmt, Args&&... args) {
        logger_->trace(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void debug(const std::string& fmt, Args&&... args) {
        logger_->debug(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(const std::string& fmt, Args&&... args) {
        logger_->info(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warn(const std::string& fmt, Args&&... args) {
        logger_->warn(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(const std::string& fmt, Args&&... args) {
        logger_->error(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void critical(const std::string& fmt, Args&&... args) {
        logger_->critical(fmt, std::forward<Args>(args)...);
    }
    
    // Set log level
    void setLevel(const std::string& level);
    
    // Flush logs
    void flush();

private:
    Logger() = default;
    ~Logger() = default;
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Logger instance
    std::shared_ptr<spdlog::logger> logger_;
    
    // Helper functions
    spdlog::level::level_enum getLogLevel(const std::string& level) const;
    void setupLogger(const std::filesystem::path& logDir,
                    spdlog::level::level_enum level);
};

// Convenience macros
#define LOG_TRACE(...) Logger::getInstance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::getInstance().critical(__VA_ARGS__)

} // namespace crypto_hft 