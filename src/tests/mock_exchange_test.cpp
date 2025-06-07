#include "MockExchangeAdapter.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

using namespace crypto_hft;

class MockExchangeTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter_ = std::make_unique<MockExchangeAdapter>();
        adapter_->connect();
    }

    void TearDown() override {
        adapter_->disconnect();
    }

    std::unique_ptr<MockExchangeAdapter> adapter_;
    std::mutex callback_mutex_;
    std::vector<OrderBookSnapshot> received_snapshots_;
    std::vector<OrderBookDelta> received_deltas_;
    std::vector<OrderResponse> received_executions_;
};

TEST_F(MockExchangeTest, BasicConnection) {
    EXPECT_TRUE(adapter_->isConnected());
    adapter_->disconnect();
    EXPECT_FALSE(adapter_->isConnected());
    EXPECT_TRUE(adapter_->connect());
    EXPECT_TRUE(adapter_->isConnected());
}

TEST_F(MockExchangeTest, MarketDataSubscription) {
    std::vector<std::string> symbols = {"BTC/USD", "ETH/USD"};
    EXPECT_TRUE(adapter_->subscribe(symbols));
    EXPECT_TRUE(adapter_->unsubscribe(symbols));
}

TEST_F(MockExchangeTest, OrderBookCallbacks) {
    std::atomic<bool> snapshot_received{false};
    std::atomic<bool> delta_received{false};

    adapter_->registerOrderBookCallback([&](const OrderBookSnapshot& snapshot) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        received_snapshots_.push_back(snapshot);
        snapshot_received = true;
    });

    adapter_->registerOrderBookDeltaCallback([&](const OrderBookDelta& delta) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        received_deltas_.push_back(delta);
        delta_received = true;
    });

    // Feed a snapshot
    OrderBookSnapshot snapshot{
        "BTC/USD",
        {{50000.0, 1.0}, {49900.0, 2.0}},
        {{50100.0, 1.0}, {50200.0, 2.0}},
        std::chrono::system_clock::now().time_since_epoch().count()
    };
    adapter_->feedOrderBookSnapshot(snapshot);

    // Feed a delta
    OrderBookDelta delta{
        "BTC/USD",
        {{49950.0, 1.5}},
        {{50150.0, 1.5}},
        std::chrono::system_clock::now().time_since_epoch().count()
    };
    adapter_->feedOrderBookDelta(delta);

    // Wait for callbacks
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(snapshot_received);
    EXPECT_TRUE(delta_received);
    EXPECT_EQ(received_snapshots_.size(), 1);
    EXPECT_EQ(received_deltas_.size(), 1);
    EXPECT_EQ(received_snapshots_[0].symbol, "BTC/USD");
    EXPECT_EQ(received_deltas_[0].symbol, "BTC/USD");
}

TEST_F(MockExchangeTest, OrderExecution) {
    std::atomic<bool> execution_received{false};
    std::atomic<bool> fill_received{false};

    adapter_->registerExecutionCallback([&](const OrderResponse& response) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        received_executions_.push_back(response);
        if (response.status == "NEW") {
            execution_received = true;
        } else if (response.status == "FILLED") {
            fill_received = true;
        }
    });

    // Submit an order
    OrderRequest request{
        "BTC/USD",
        OrderRequest::Side::BUY,
        OrderRequest::Type::LIMIT,
        50000.0,
        1.0,
        "test_order_1"
    };
    std::string orderId = adapter_->submitOrder(request);
    EXPECT_FALSE(orderId.empty());

    // Wait for execution and fill
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_TRUE(execution_received);
    EXPECT_TRUE(fill_received);
    EXPECT_EQ(received_executions_.size(), 2);
    EXPECT_EQ(received_executions_[0].status, "NEW");
    EXPECT_EQ(received_executions_[1].status, "FILLED");
    EXPECT_EQ(received_executions_[1].filledAmount, 1.0);
    EXPECT_EQ(received_executions_[1].fillPrice, 50000.0);
}

TEST_F(MockExchangeTest, OrderCancellation) {
    std::atomic<bool> cancel_received{false};

    adapter_->registerExecutionCallback([&](const OrderResponse& response) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        received_executions_.push_back(response);
        if (response.status == "CANCELED") {
            cancel_received = true;
        }
    });

    // Submit an order
    OrderRequest request{
        "BTC/USD",
        OrderRequest::Side::BUY,
        OrderRequest::Type::LIMIT,
        50000.0,
        1.0,
        "test_order_2"
    };
    std::string orderId = adapter_->submitOrder(request);
    EXPECT_FALSE(orderId.empty());

    // Cancel the order
    EXPECT_TRUE(adapter_->cancelOrder(orderId));

    // Wait for cancellation
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(cancel_received);
    EXPECT_EQ(received_executions_.back().status, "CANCELED");
}

TEST_F(MockExchangeTest, MarketDataSimulation) {
    std::atomic<int> updates_received{0};

    adapter_->registerOrderBookDeltaCallback([&](const OrderBookDelta& delta) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        received_deltas_.push_back(delta);
        updates_received++;
    });

    // Start market data simulation
    adapter_->startMarketDataSimulation();

    // Feed some market data
    MarketData data{
        "BTC/USD",
        50000.0,
        1.0,
        50100.0,
        1.0,
        std::chrono::system_clock::now().time_since_epoch().count()
    };
    adapter_->feedMarketData(data);

    // Wait for updates
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_GT(updates_received, 0);
    EXPECT_EQ(received_deltas_.back().symbol, "BTC/USD");
}

TEST_F(MockExchangeTest, AccountInformation) {
    EXPECT_DOUBLE_EQ(adapter_->getBalance("BTC"), 1000.0);
    
    auto balances = adapter_->getAllBalances();
    EXPECT_EQ(balances.size(), 3);
    EXPECT_EQ(balances[0].first, "BTC");
    EXPECT_EQ(balances[1].first, "ETH");
    EXPECT_EQ(balances[2].first, "USDT");
    
    EXPECT_EQ(adapter_->getName(), "MockExchange");
    EXPECT_TRUE(adapter_->supportsMargin());
    EXPECT_DOUBLE_EQ(adapter_->getFeeRate("BTC/USD"), 0.001);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 