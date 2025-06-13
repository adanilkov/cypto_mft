#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/TradingSystem.hpp"
#include "core/MarketDataEngine.hpp"
#include "core/OrderBook.hpp"
#include "exchanges/CoinbaseAdapter.hpp"
#include "infra/ConfigManager.hpp"
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace crypto_hft;
using namespace testing;
using json = nlohmann::json;

class OrderBookPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Load config from file instead of string
        config_ = ConfigManager::create();
        ASSERT_TRUE(config_->loadFromFile("src/configs/default_config.yaml"));

        trading_system_ = std::make_unique<TradingSystem>(config_);
        ASSERT_TRUE(trading_system_->initialize());
        
        // Set up order book verification
        order_book_updates_ = 0;
        last_snapshot_received_ = false;
        last_delta_received_ = false;
        
        // Register callbacks to verify order book updates
        auto market_data_engine = trading_system_->getMarketDataEngine();
        if (market_data_engine) {
            market_data_engine->registerCallback(
                [this](const MarketUpdate& update) {
                    if (update.isSnapshot) {
                        OrderBookSnapshot snapshot;
                        snapshot.symbol = update.symbol;
                        snapshot.timestamp = update.timestamp;
                        snapshot.bids.push_back({update.bidPrice, update.bidSize});
                        snapshot.asks.push_back({update.askPrice, update.askSize});
                        verifyOrderBookSnapshot(update.symbol, snapshot);
                    } else {
                        OrderBookDelta delta;
                        delta.symbol = update.symbol;
                        delta.timestamp = update.timestamp;
                        delta.bidUpdates.push_back({update.bidPrice, update.bidSize});
                        delta.askUpdates.push_back({update.askPrice, update.askSize});
                        verifyOrderBookDelta(update.symbol, delta);
                    }
                }
            );
        }
    }
    
    void TearDown() override {
        if (trading_system_) {
            trading_system_->stop();
        }
    }
    
    void verifyOrderBookSnapshot([[maybe_unused]] const std::string& symbol, const OrderBookSnapshot& snapshot) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Verify snapshot data
        EXPECT_FALSE(snapshot.bids.empty()) << "Snapshot should contain bids";
        EXPECT_FALSE(snapshot.asks.empty()) << "Snapshot should contain asks";
        
        // Verify bid/ask ordering
        for (size_t i = 1; i < snapshot.bids.size(); ++i) {
            EXPECT_GE(snapshot.bids[i-1].first, snapshot.bids[i].first) 
                << "Bids should be in descending order";
        }
        
        for (size_t i = 1; i < snapshot.asks.size(); ++i) {
            EXPECT_LE(snapshot.asks[i-1].first, snapshot.asks[i].first) 
                << "Asks should be in ascending order";
        }
        
        // Verify no price crossing
        if (!snapshot.bids.empty() && !snapshot.asks.empty()) {
            EXPECT_LE(snapshot.bids[0].first, snapshot.asks[0].first) 
                << "Best bid should not cross best ask";
        }
        
        last_snapshot_received_ = true;
        order_book_updates_++;
        cv_.notify_one();
    }
    
    void verifyOrderBookDelta([[maybe_unused]] const std::string& symbol, const OrderBookDelta& delta) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Verify delta data
        EXPECT_FALSE(delta.bidUpdates.empty() || delta.askUpdates.empty()) 
            << "Delta should contain updates";
        
        // Verify bid/ask ordering in updates
        for (size_t i = 1; i < delta.bidUpdates.size(); ++i) {
            EXPECT_GE(delta.bidUpdates[i-1].first, delta.bidUpdates[i].first) 
                << "Bid updates should be in descending order";
        }
        
        for (size_t i = 1; i < delta.askUpdates.size(); ++i) {
            EXPECT_LE(delta.askUpdates[i-1].first, delta.askUpdates[i].first) 
                << "Ask updates should be in ascending order";
        }
        
        last_delta_received_ = true;
        order_book_updates_++;
        cv_.notify_one();
    }
    
    bool waitForUpdates(int expected_updates, std::chrono::seconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [this, expected_updates]() {
            return order_book_updates_ >= expected_updates;
        });
    }
    
    void verifyOrderBookUpdate(const std::string& symbol, 
                             const std::vector<std::pair<double, double>>& expected_bids,
                             const std::vector<std::pair<double, double>>& expected_asks) {
        auto market_data = trading_system_->getMarketDataEngine();
        ASSERT_NE(market_data, nullptr);

        auto order_book = market_data->getOrderBook(symbol);
        ASSERT_NE(order_book, nullptr);

        // Wait for order book to be populated (with timeout)
        const int max_attempts = 10;
        const int delay_ms = 100;
        int attempts = 0;
        
        while (attempts < max_attempts) {
            auto bids = order_book->getBids();
            auto asks = order_book->getAsks();
            if (!bids.empty() && !asks.empty()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            attempts++;
        }

        ASSERT_LT(attempts, max_attempts) << "Order book not populated within timeout";

        // Verify bids
        const auto& bids = order_book->getBids();
        ASSERT_EQ(bids.size(), expected_bids.size());
        size_t i = 0;
        for (const auto& [price, size] : bids) {
            EXPECT_DOUBLE_EQ(price, expected_bids[i].first) 
                << "Bid price mismatch at level " << i;
            EXPECT_DOUBLE_EQ(size, expected_bids[i].second) 
                << "Bid size mismatch at level " << i;
            i++;
        }

        // Verify asks
        const auto& asks = order_book->getAsks();
        ASSERT_EQ(asks.size(), expected_asks.size());
        i = 0;
        for (const auto& [price, size] : asks) {
            EXPECT_DOUBLE_EQ(price, expected_asks[i].first) 
                << "Ask price mismatch at level " << i;
            EXPECT_DOUBLE_EQ(size, expected_asks[i].second) 
                << "Ask size mismatch at level " << i;
            i++;
        }
    }
    
    std::shared_ptr<ConfigManager> config_;
    std::unique_ptr<TradingSystem> trading_system_;
    
    std::atomic<int> order_book_updates_;
    std::atomic<bool> last_snapshot_received_;
    std::atomic<bool> last_delta_received_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

TEST_F(OrderBookPipelineTest, CoinbaseOrderBookUpdate) {
    // Start the trading system
    ASSERT_TRUE(trading_system_->start());

    // Get the market data engine
    auto market_data = trading_system_->getMarketDataEngine();
    ASSERT_NE(market_data, nullptr);

    // Create a market update for the snapshot
    MarketUpdate snapshot_update;
    snapshot_update.symbol = "BTC-USD";
    snapshot_update.isSnapshot = true;
    snapshot_update.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Add bid/ask levels
    snapshot_update.bidPrice = 50000.00;
    snapshot_update.bidSize = 1.5;
    snapshot_update.askPrice = 50001.00;
    snapshot_update.askSize = 1.0;

    // Trigger the update through the callback
    if (market_data->getUpdateCallback()) {
        market_data->getUpdateCallback()(snapshot_update);
    }

    // Verify the order book was updated correctly
    std::vector<std::pair<double, double>> expected_bids = {
        {50000.00, 1.5}
    };

    std::vector<std::pair<double, double>> expected_asks = {
        {50001.00, 1.0}
    };

    verifyOrderBookUpdate("BTC-USD", expected_bids, expected_asks);

    // Create a market update for the delta
    MarketUpdate delta_update;
    delta_update.symbol = "BTC-USD";
    delta_update.isSnapshot = false;
    delta_update.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Update bid/ask levels
    delta_update.bidPrice = 49999.00;
    delta_update.bidSize = 3.0;
    delta_update.askPrice = 50001.00;
    delta_update.askSize = 0.5;

    // Trigger the update through the callback
    if (market_data->getUpdateCallback()) {
        market_data->getUpdateCallback()(delta_update);
    }

    // Update expected values
    expected_bids[0] = {49999.00, 3.0};  // Updated bid
    expected_asks[0] = {50001.00, 0.5};  // Updated ask

    verifyOrderBookUpdate("BTC-USD", expected_bids, expected_asks);
}

int main(int argc, char **argv) {
    // Set up spdlog to print debug logs to stdout
    auto logger = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    spdlog::flush_on(spdlog::level::debug);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 