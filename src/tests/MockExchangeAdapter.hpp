#pragma once

#include "../exchanges/IExchangeAdapter.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace crypto_hft {

class MockExchangeAdapter : public IExchangeAdapter {
public:
    MockExchangeAdapter();
    ~MockExchangeAdapter() override;

    // Prevent copying
    MockExchangeAdapter(const MockExchangeAdapter&) = delete;
    MockExchangeAdapter& operator=(const MockExchangeAdapter&) = delete;

    // IExchangeAdapter interface implementation
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    
    // Market data subscription
    bool subscribe(const std::vector<std::string>& symbols) override;
    bool unsubscribe(const std::vector<std::string>& symbols) override;
    
    // Order book management
    bool requestOrderBookSnapshot(const std::string& symbol) override;
    void registerOrderBookCallback(std::function<void(const OrderBookSnapshot&)> callback) override;
    void registerOrderBookDeltaCallback(std::function<void(const OrderBookDelta&)> callback) override;
    
    // Order management
    std::string submitOrder(const OrderRequest& request) override;
    bool cancelOrder(const std::string& orderId) override;
    bool modifyOrder(const std::string& orderId, double newPrice, double newSize) override;
    
    // Execution reports
    void registerExecutionCallback(std::function<void(const OrderResponse&)> callback) override;
    
    // Account information
    double getBalance(const std::string& asset) const override;
    std::vector<std::pair<std::string, double>> getAllBalances() const override;
    
    // Exchange information
    std::string getName() const override;
    bool supportsMargin() const override;
    double getFeeRate(const std::string& symbol) const override;

    // Mock-specific methods
    void feedMarketData(const MarketData& data);
    void feedOrderBookSnapshot(const OrderBookSnapshot& snapshot);
    void feedOrderBookDelta(const OrderBookDelta& delta);
    void setSimulatedLatency(std::chrono::milliseconds latency);
    void loadMarketDataFromFile(const std::string& filename);
    void startMarketDataSimulation();
    void stopMarketDataSimulation();

private:
    // Internal state
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::chrono::milliseconds simulated_latency_;
    
    // Callbacks
    std::function<void(const OrderBookSnapshot&)> order_book_callback_;
    std::function<void(const OrderBookDelta&)> order_book_delta_callback_;
    std::function<void(const OrderResponse&)> execution_callback_;
    
    // Market data simulation
    std::thread market_data_thread_;
    std::queue<nlohmann::json> market_data_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Order tracking
    struct OrderState {
        OrderRequest request;
        std::string status;
        double filled_amount;
        double fill_price;
    };
    std::unordered_map<std::string, OrderState> orders_;  // orderId -> state
    std::mutex orders_mutex_;
    
    // Internal methods
    void marketDataSimulationLoop();
    void processOrder(const OrderRequest& request);
    void simulateOrderExecution(const std::string& orderId);
    void loadMarketDataFromJson(const nlohmann::json& data);
};
} 