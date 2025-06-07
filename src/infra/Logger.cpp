#include "Logger.hpp"

namespace crypto_hft {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::init(const std::string& filename, size_t max_size, size_t max_files) {
    try {
        // Create async logger with rotating file sink
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            filename, max_size, max_files);

        // Create async logger with queue size of 8192 items
        logger_ = std::make_shared<spdlog::async_logger>(
            "crypto_hft",
            sink,
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);

        // Set pattern
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

        // Set level
        logger_->set_level(spdlog::level::debug);

        // Flush on every log
        logger_->flush_on(spdlog::level::debug);

        spdlog::register_logger(logger_);
    } catch (const spdlog::spdlog_ex& ex) {
        throw std::runtime_error("Failed to initialize logger: " + std::string(ex.what()));
    }
}

} // namespace crypto_hft 