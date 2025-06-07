#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
#include <thread>
#include <condition_variable>

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
    MarketDataEngine();
    ~MarketDataEngine();

    // Initialize connections to exchanges
    void initialize(const std::vector<std::string>& symbols);
    
    // Subscribe to market data for specific symbols
    void subscribe(const std::vector<std::string>& symbols);
    
    // Register callback for market updates
    void registerCallback(std::function<void(const MarketUpdate&)> callback);
    
    // Get current order book for a symbol
    std::shared_ptr<OrderBook> getOrderBook(const std::string& symbol);

private:
    // Internal thread functions
    void receiverThread(std::shared_ptr<IExchangeAdapter> adapter);
    void dispatcherThread();

    // Exchange adapters
    std::vector<std::shared_ptr<IExchangeAdapter>> adapters_;
    
    // Order books per symbol
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> orderBooks_;
    
    // Thread-safe queue for market updates
    struct UpdateQueue {
        std::queue<MarketUpdate> queue;
        std::mutex mutex;
        std::condition_variable cv;
    };
    std::vector<std::unique_ptr<UpdateQueue>> updateQueues_;
    
    // Callback for market updates
    std::function<void(const MarketUpdate&)> updateCallback_;
    
    // Thread management
    std::vector<std::thread> receiverThreads_;
    std::thread dispatcherThread_;
    std::atomic<bool> running_;
    
    // Synchronization
    std::mutex orderBooksMutex_;
};

} // namespace crypto_hft 