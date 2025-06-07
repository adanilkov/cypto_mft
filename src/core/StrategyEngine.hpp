#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "OrderBook.hpp"
#include "../models/SignalGenerator.hpp"
#include "MarketDataEngine.hpp"

namespace crypto_hft {

struct TradeSignal {
    std::string symbol;
    enum class Side { BUY, SELL } side;
    double price;
    double size;
    double zscore;  // For statistical arbitrage strategies
    int64_t timestamp;
};

class IStrategy {
public:
    virtual ~IStrategy() = default;
    virtual void onMarketUpdate(const std::string& symbol, 
                              const std::shared_ptr<OrderBook>& orderBook) = 0;
    virtual std::optional<TradeSignal> generateSignal() = 0;
    virtual void onExecutionReport(const std::string& orderId, 
                                 const std::string& status,
                                 double filledAmount,
                                 double fillPrice) = 0;
};

class StrategyEngine {
public:
    StrategyEngine();
    ~StrategyEngine();

    // Initialize with market data engine
    void initialize(std::shared_ptr<MarketDataEngine> marketDataEngine);
    
    // Add a strategy instance
    void addStrategy(std::shared_ptr<IStrategy> strategy);
    
    // Start strategy processing
    void start();
    
    // Stop strategy processing
    void stop();
    
    // Register callback for trade signals
    void registerSignalCallback(std::function<void(const TradeSignal&)> callback);

private:
    // Internal thread function
    void strategyThread(std::shared_ptr<IStrategy> strategy);
    
    // Market data engine
    std::shared_ptr<MarketDataEngine> marketDataEngine_;
    
    // Strategy instances
    std::vector<std::shared_ptr<IStrategy>> strategies_;
    
    // Thread management
    std::vector<std::thread> strategyThreads_;
    std::atomic<bool> running_;
    
    // Signal callback
    std::function<void(const TradeSignal&)> signalCallback_;
    
    // Thread-safe queue for market updates
    struct UpdateQueue {
        std::queue<std::pair<std::string, std::shared_ptr<OrderBook>>> queue;
        std::mutex mutex;
        std::condition_variable cv;
    };
    std::unordered_map<std::shared_ptr<IStrategy>, 
                      std::unique_ptr<UpdateQueue>> updateQueues_;
};

} // namespace crypto_hft 