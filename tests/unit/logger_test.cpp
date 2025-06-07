#include <gtest/gtest.h>
#include "../../src/infra/Logger.hpp"
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

namespace crypto_hft {
namespace test {

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary log file
        log_file_ = std::filesystem::temp_directory_path() / "crypto_hft_test.log";
        
        // Print the log file path
        std::cout << "Log file path: " << log_file_.string() << std::endl;
        
        // Remove existing file/directory if it exists
        if (std::filesystem::exists(log_file_)) {
            if (std::filesystem::is_directory(log_file_)) {
                std::filesystem::remove_all(log_file_);
            } else {
                std::filesystem::remove(log_file_);
            }
        }
        
        // Drop existing logger if it exists
        try {
            spdlog::drop("crypto_hft");
        } catch (...) {
            // Ignore if logger doesn't exist
        }
        
        // Initialize logger with synchronous mode
        Logger::getInstance().initialize(log_file_.string(), "trace", false);
    }

    void TearDown() override {
        // Flush any pending logs
        Logger::getInstance().flush();
        
        // Drop the logger
        try {
            spdlog::drop("crypto_hft");
        } catch (...) {
            // Ignore if logger doesn't exist
        }
        
        // Clean up log file
        if (std::filesystem::exists(log_file_)) {
            std::filesystem::remove(log_file_);
        }
    }

    std::filesystem::path log_file_;
};

TEST_F(LoggerTest, BasicLogging) {
    LOG_INFO("Test info message");
    LOG_WARN("Test warning message");
    LOG_ERROR("Test error message");

    // Give some time for async logger to write
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Logger::getInstance().flush();

    // Read log file
    std::ifstream log(log_file_);
    std::stringstream buffer;
    buffer << log.rdbuf();
    std::string content = buffer.str();

    // Verify log messages
    EXPECT_TRUE(content.find("Test info message") != std::string::npos);
    EXPECT_TRUE(content.find("Test warning message") != std::string::npos);
    EXPECT_TRUE(content.find("Test error message") != std::string::npos);
}

TEST_F(LoggerTest, MultiThreadedLogging) {
    constexpr int num_threads = 5;
    constexpr int messages_per_thread = 100;
    std::vector<std::thread> threads;

    // Create threads that log messages
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, messages_per_thread]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                LOG_INFO("Thread " + std::to_string(i) + " message " + std::to_string(j));
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Give some time for async logger to write
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Logger::getInstance().flush();

    // Read log file
    std::ifstream log(log_file_);
    std::stringstream buffer;
    buffer << log.rdbuf();
    std::string content = buffer.str();

    // Count messages from each thread
    std::vector<int> message_counts(num_threads, 0);
    for (int i = 0; i < num_threads; ++i) {
        for (int j = 0; j < messages_per_thread; ++j) {
            std::string expected = "Thread " + std::to_string(i) + " message " + std::to_string(j);
            if (content.find(expected) != std::string::npos) {
                message_counts[i]++;
            }
        }
    }

    // Verify all messages were logged
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(message_counts[i], messages_per_thread) 
            << "Thread " << i << " missing messages";
    }
}

TEST_F(LoggerTest, LogLevels) {
    LOG_TRACE("Trace message");
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    LOG_CRITICAL("Critical message");

    // Give some time for async logger to write
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Logger::getInstance().flush();

    // Read log file
    std::ifstream log(log_file_);
    std::stringstream buffer;
    buffer << log.rdbuf();
    std::string content = buffer.str();

    // Verify log levels
    EXPECT_TRUE(content.find("[trace]") != std::string::npos);
    EXPECT_TRUE(content.find("[debug]") != std::string::npos);
    EXPECT_TRUE(content.find("[info]") != std::string::npos);
    EXPECT_TRUE(content.find("[warning]") != std::string::npos);
    EXPECT_TRUE(content.find("[error]") != std::string::npos);
    EXPECT_TRUE(content.find("[critical]") != std::string::npos);
}

} // namespace test
} // namespace crypto_hft 