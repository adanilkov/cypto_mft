#include "Logger.hpp"
#include <spdlog/sinks/basic_file_sink.h>

namespace crypto_hft {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::filesystem::path& logPath, const std::string& logLevel, bool use_async) {
    try {
        // Drop existing logger if it exists
        try {
            spdlog::drop("crypto_hft");
        } catch (...) {
            // Ignore if logger doesn't exist
        }

        // Create file sink
        std::shared_ptr<spdlog::sinks::sink> sink;
        if (use_async) {
            // Use rotating file sink for production
            sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logPath.string(), 1024 * 1024 * 5, 3);  // 5MB per file, 3 files max
        } else {
            // Use basic file sink for testing
            sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
        }

        if (use_async) {
            // Initialize thread pool for async logging
            spdlog::init_thread_pool(8192, 1);
            
            // Create async logger
            logger_ = std::make_shared<spdlog::async_logger>(
                "crypto_hft",
                sink,
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
        } else {
            // Create synchronous logger
            logger_ = std::make_shared<spdlog::logger>("crypto_hft", sink);
        }

        // Set pattern
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

        // Set level
        logger_->set_level(getLogLevel(logLevel));

        // Flush on every log
        logger_->flush_on(spdlog::level::debug);

        spdlog::register_logger(logger_);
    } catch (const spdlog::spdlog_ex& ex) {
        throw std::runtime_error("Failed to initialize logger: " + std::string(ex.what()));
    }
}

spdlog::level::level_enum Logger::getLogLevel(const std::string& level) const {
    if (level == "trace") return spdlog::level::trace;
    if (level == "debug") return spdlog::level::debug;
    if (level == "info") return spdlog::level::info;
    if (level == "warn") return spdlog::level::warn;
    if (level == "error") return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    return spdlog::level::info;  // Default to info level
}

void Logger::setLevel(const std::string& level) {
    if (logger_) {
        logger_->set_level(getLogLevel(level));
    }
}

void Logger::flush() {
    if (logger_) {
        logger_->flush();
    }
}

Logger::~Logger() {
    if (logger_) {
        try {
            logger_->flush();
            spdlog::drop(logger_->name());
        } catch (...) {
            // Ignore any errors during cleanup
        }
    }
}

} // namespace crypto_hft 