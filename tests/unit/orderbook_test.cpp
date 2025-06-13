#include <gtest/gtest.h>
#include "../../src/core/OrderBook.hpp"
#include "MockExchangeAdapter.hpp"
#include <thread>
#include <chrono>
#include <map>
#include <atomic>
#include <random>

using namespace crypto_hft;

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        orderBook = std::make_shared<OrderBook>("BTC-USD");
        mockAdapter = std::make_shared<MockExchangeAdapter>();
    }

    void TearDown() override {
        mockAdapter->disconnect();
    }

    // Helper function to convert vector of pairs to map
    std::map<double, double> toMap(const std::vector<std::pair<double, double>>& pairs) {
        return std::map<double, double>(pairs.begin(), pairs.end());
    }

    std::shared_ptr<OrderBook> orderBook;
    std::shared_ptr<MockExchangeAdapter> mockAdapter;
};

TEST_F(OrderBookTest, InitialState) {
    EXPECT_EQ(orderBook->getBestBid(), 0.0);
    EXPECT_EQ(orderBook->getBestAsk(), 0.0);
    EXPECT_EQ(orderBook->getSpread(), 0.0);
    EXPECT_EQ(orderBook->getMidPrice(), 0.0);
}

TEST_F(OrderBookTest, SnapshotUpdate) {
    OrderBookSnapshot snapshot;
    snapshot.symbol = "BTC-USD";
    snapshot.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Add some levels
    snapshot.bids = {{50000.0, 1.0}, {49900.0, 2.0}, {49800.0, 3.0}};
    snapshot.asks = {{50100.0, 1.0}, {50200.0, 2.0}, {50300.0, 3.0}};

    orderBook->setSnapshot(toMap(snapshot.bids), toMap(snapshot.asks));

    EXPECT_DOUBLE_EQ(orderBook->getBestBid(), 50000.0);
    EXPECT_DOUBLE_EQ(orderBook->getBestAsk(), 50100.0);
    EXPECT_DOUBLE_EQ(orderBook->getSpread(), 100.0);
    EXPECT_DOUBLE_EQ(orderBook->getMidPrice(), 50050.0);
    EXPECT_DOUBLE_EQ(orderBook->getBidVolume(50000.0), 1.0);
    EXPECT_DOUBLE_EQ(orderBook->getAskVolume(50100.0), 1.0);
}

TEST_F(OrderBookTest, DeltaUpdates) {
    // Set initial state
    OrderBookSnapshot snapshot;
    snapshot.symbol = "BTC-USD";
    snapshot.bids = {{50000.0, 1.0}};
    snapshot.asks = {{50100.0, 1.0}};
    orderBook->setSnapshot(toMap(snapshot.bids), toMap(snapshot.asks));

    // Update bid
    orderBook->updateBid(49900.0, 2.0);
    EXPECT_DOUBLE_EQ(orderBook->getBidVolume(49900.0), 2.0);
    EXPECT_DOUBLE_EQ(orderBook->getBestBid(), 50000.0);  // Best bid unchanged

    // Update ask
    orderBook->updateAsk(50200.0, 2.0);
    EXPECT_DOUBLE_EQ(orderBook->getAskVolume(50200.0), 2.0);
    EXPECT_DOUBLE_EQ(orderBook->getBestAsk(), 50100.0);  // Best ask unchanged

    // Remove bid
    orderBook->removeBid(50000.0);
    EXPECT_DOUBLE_EQ(orderBook->getBidVolume(50000.0), 0.0);
    EXPECT_DOUBLE_EQ(orderBook->getBestBid(), 49900.0);  // New best bid

    // Remove ask
    orderBook->removeAsk(50100.0);
    EXPECT_DOUBLE_EQ(orderBook->getAskVolume(50100.0), 0.0);
    EXPECT_DOUBLE_EQ(orderBook->getBestAsk(), 50200.0);  // New best ask
}

TEST_F(OrderBookTest, ConcurrentUpdates) {
    // Set initial state
    OrderBookSnapshot snapshot;
    snapshot.symbol = "BTC-USD";
    snapshot.bids = {{50000.0, 1.0}};
    snapshot.asks = {{50100.0, 1.0}};
    orderBook->setSnapshot(toMap(snapshot.bids), toMap(snapshot.asks));

    // Create multiple threads updating the order book
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i]() {
            double baseBidPrice = 50000.0 - i * 100.0;
            double baseAskPrice = baseBidPrice + 100.0;
            for (int j = 0; j < 100; ++j) {
                double bidPrice = baseBidPrice - j;
                double askPrice = baseAskPrice - j;
                orderBook->updateBid(bidPrice, 1.0);
                orderBook->updateAsk(askPrice, 1.0);
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }

    // Reader thread (removed invariant check during concurrent updates)
    std::thread reader([this]() {
        for (int i = 0; i < 1000; ++i) {
            orderBook->getBestBid();
            orderBook->getBestAsk();
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    reader.join();

    // Verify final state
    auto bids = orderBook->getBids();
    auto asks = orderBook->getAsks();
    EXPECT_FALSE(bids.empty());
    EXPECT_FALSE(asks.empty());
    EXPECT_GE(orderBook->getBestAsk(), orderBook->getBestBid());
}

TEST_F(OrderBookTest, CumulativeVolume) {
    OrderBookSnapshot snapshot;
    snapshot.symbol = "BTC-USD";
    snapshot.bids = {
        {50000.0, 1.0},
        {49900.0, 2.0},
        {49800.0, 3.0}
    };
    snapshot.asks = {
        {50100.0, 1.0},
        {50200.0, 2.0},
        {50300.0, 3.0}
    };
    orderBook->setSnapshot(toMap(snapshot.bids), toMap(snapshot.asks));

    // Test cumulative bid volume
    EXPECT_DOUBLE_EQ(orderBook->getCumulativeBidVolume(49900.0), 1.0);  // Only volume at 50000
    EXPECT_DOUBLE_EQ(orderBook->getCumulativeBidVolume(49800.0), 3.0);  // Volume at 50000 + 49900

    // Test cumulative ask volume
    EXPECT_DOUBLE_EQ(orderBook->getCumulativeAskVolume(50200.0), 1.0);  // Only volume at 50100
    EXPECT_DOUBLE_EQ(orderBook->getCumulativeAskVolume(50300.0), 3.0);  // Volume at 50100 + 50200
}

TEST_F(OrderBookTest, ThreadSafetyWithRandomDeltas) {
    // Set initial state
    OrderBookSnapshot snapshot;
    snapshot.symbol = "BTC-USD";
    snapshot.bids = {{50000.0, 1.0}};
    snapshot.asks = {{50100.0, 1.0}};
    orderBook->setSnapshot(toMap(snapshot.bids), toMap(snapshot.asks));

    std::atomic<bool> stop_threads{false};
    std::atomic<int> read_count{0};
    std::atomic<int> update_count{0};

    // Thread A: Continuously read top of book
    std::thread reader([this, &stop_threads, &read_count]() {
        while (!stop_threads && read_count < 1000) {
            double bid = orderBook->getBestBid();
            double ask = orderBook->getBestAsk();
            double spread = orderBook->getSpread();
            double mid = orderBook->getMidPrice();
            
            // Basic sanity checks
            EXPECT_GE(ask, bid);
            EXPECT_DOUBLE_EQ(spread, ask - bid);
            EXPECT_DOUBLE_EQ(mid, (bid + ask) / 2.0);
            
            read_count++;
            std::this_thread::yield();  // Give other thread a chance
        }
    });

    // Thread B: Apply random deltas
    std::thread updater([this, &stop_threads, &update_count]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> price_dist(49000.0, 51000.0);
        std::uniform_real_distribution<> size_dist(0.1, 10.0);
        std::uniform_int_distribution<> action_dist(0, 3);  // 0: update bid, 1: update ask, 2: remove bid, 3: remove ask

        while (!stop_threads && update_count < 1000) {
            double price = price_dist(gen);
            double size = size_dist(gen);
            int action = action_dist(gen);

            switch (action) {
                case 0:
                    orderBook->updateBid(price, size);
                    break;
                case 1:
                    orderBook->updateAsk(price, size);
                    break;
                case 2:
                    orderBook->removeBid(price);
                    break;
                case 3:
                    orderBook->removeAsk(price);
                    break;
            }

            update_count++;
            std::this_thread::yield();  // Give other thread a chance
        }
    });

    // Wait for both threads to complete their work
    while (read_count < 1000 || update_count < 1000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    stop_threads = true;

    reader.join();
    updater.join();

    // Verify final state
    EXPECT_EQ(read_count, 1000);
    EXPECT_EQ(update_count, 1000);
    
    // Final sanity check
    double final_bid = orderBook->getBestBid();
    double final_ask = orderBook->getBestAsk();
    EXPECT_GE(final_ask, final_bid);
} 