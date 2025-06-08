#include "../exchanges/CoinbaseAdapter.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <iostream>

using namespace crypto_hft;

class CoinbaseWebSocketTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter_ = std::make_unique<CoinbaseAdapter>("../tests/unit/test_config.yaml");
        
        // Set up message callback
        adapter_->registerOrderBookCallback([this](const OrderBookSnapshot& snapshot) {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            std::cout << "Received ticker data for " << snapshot.symbol << std::endl;
            std::cout << "Best bid: " << snapshot.bids[0].first << " @ " << snapshot.bids[0].second << std::endl;
            std::cout << "Best ask: " << snapshot.asks[0].first << " @ " << snapshot.asks[0].second << std::endl;
            message_received_ = true;
            cv_.notify_one();
        });
    }

    void TearDown() override {
        if (adapter_) {
            adapter_->disconnect();
        }
    }

    std::unique_ptr<CoinbaseAdapter> adapter_;
    std::mutex callback_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> message_received_{false};
};

TEST_F(CoinbaseWebSocketTest, BasicConnectionAndSubscription) {
    std::cout << "Starting Coinbase WebSocket test..." << std::endl;

    // Connect to Coinbase
    ASSERT_TRUE(adapter_->connect()) << "Failed to connect to Coinbase";
    EXPECT_TRUE(adapter_->isConnected()) << "Connection status is false after connect()";

    // Subscribe to BTC-USD ticker
    std::vector<std::string> symbols = {"BTC-USD"};
    ASSERT_TRUE(adapter_->subscribe(symbols)) << "Failed to subscribe to BTC-USD";

    std::cout << "Waiting for ticker data (timeout: 10 seconds)..." << std::endl;

    // Wait for some messages (up to 10 seconds)
    std::unique_lock<std::mutex> lock(callback_mutex_);
    bool received = cv_.wait_for(lock, std::chrono::seconds(10), 
        [this]() { return message_received_.load(); });

    EXPECT_TRUE(received) << "No ticker messages received within timeout period";
    
    if (received) {
        std::cout << "Successfully received ticker data!" << std::endl;
    }
} 