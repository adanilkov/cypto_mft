#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>

namespace crypto_hft {

class Logger {
public:
    static Logger& getInstance();

    // Initialize logger
    void initialize(const std::filesystem::path& logDir,
                   const std::string& logLevel = "info",
                   bool use_async = true);
    
    // Logging methods
    void trace(const std::string& msg) {
        logger_->trace(msg);
    }
    
    void debug(const std::string& msg) {
        logger_->debug(msg);
    }
    
    void info(const std::string& msg) {
        logger_->info(msg);
    }
    
    void warn(const std::string& msg) {
        logger_->warn(msg);
    }
    
    void error(const std::string& msg) {
        logger_->error(msg);
    }
    
    void critical(const std::string& msg) {
        logger_->critical(msg);
    }
    
    // Set log level
    void setLevel(const std::string& level);
    
    // Flush logs
    void flush();

private:
    Logger() = default;
    ~Logger();  // Declare destructor but don't default it
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Logger instance
    std::shared_ptr<spdlog::logger> logger_;
    
    // Helper functions
    spdlog::level::level_enum getLogLevel(const std::string& level) const;
    void setupLogger(const std::filesystem::path& logDir,
                    spdlog::level::level_enum level,
                    bool use_async);
};

// Convenience macros
#define LOG_TRACE(msg) Logger::getInstance().trace(msg)
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg)
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARN(msg) Logger::getInstance().warn(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) Logger::getInstance().critical(msg)

} // namespace crypto_hft 