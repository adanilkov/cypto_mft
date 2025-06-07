#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace crypto_hft {

class Logger {
public:
    static Logger& getInstance();

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Initialize logger with file output
    void init(const std::string& filename, size_t max_size = 5 * 1024 * 1024, size_t max_files = 3);

    // Get the underlying spdlog logger
    std::shared_ptr<spdlog::logger> getLogger() const { return logger_; }

private:
    Logger() = default;
    ~Logger() = default;

    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace crypto_hft

// Logging macros
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(crypto_hft::Logger::getInstance().getLogger(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(crypto_hft::Logger::getInstance().getLogger(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(crypto_hft::Logger::getInstance().getLogger(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(crypto_hft::Logger::getInstance().getLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(crypto_hft::Logger::getInstance().getLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(crypto_hft::Logger::getInstance().getLogger(), __VA_ARGS__) 