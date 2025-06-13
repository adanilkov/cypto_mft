#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <functional>
#include <thread>
#include <concurrentqueue.h>
#include <spdlog/spdlog.h>

#include "../exchanges/IExchangeAdapter.hpp"
#include "OrderBook.hpp"

namespace crypto_hft {

struct MarketUpdate {
    std::string symbol;
    double bidPrice;
    double bidSize;
    double askPrice;
    double askSize;
    int64_t timestamp;
    bool isSnapshot;
};

class MarketDataEngine {
public:
    explicit MarketDataEngine(std::vector<std::shared_ptr<IExchangeAdapter>> adapters);
    ~MarketDataEngine();

    void initialize(const std::vector<std::string>& symbols);
    void stop();
    void registerCallback(std::function<void(const MarketUpdate&)> callback);
    std::shared_ptr<OrderBook> getOrderBook(const std::string& symbol);
    const std::function<void(const MarketUpdate&)>& getUpdateCallback() const { return updateCallback_; }

private:
    // Internal thread functions
    void receiverThread(std::shared_ptr<IExchangeAdapter> adapter);
    void dispatcherThread();
    void handleOrderBookSnapshot(const OrderBookSnapshot& snapshot);
    void handleOrderBookDelta(const OrderBookDelta& delta);

    // Exchange adapters
    std::vector<std::shared_ptr<IExchangeAdapter>> adapters_;
    
    // Order books per symbol
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> orderBooks_;
    
    // Lock-free queue for market updates
    moodycamel::ConcurrentQueue<MarketUpdate> updateQueue_;
    
    // Callback for market updates
    std::function<void(const MarketUpdate&)> updateCallback_;
    
    // Thread management
    std::vector<std::thread> receiverThreads_;
    std::thread dispatcherThread_;
    std::atomic<bool> running_;
    
    // Synchronization
    std::mutex orderBooksMutex_;
    
    // Logging
    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace crypto_hft 