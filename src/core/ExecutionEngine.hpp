#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "../exchanges/IExchangeAdapter.hpp"

namespace crypto_hft {

// Using OrderRequest and OrderResponse from IExchangeAdapter.hpp

class ExecutionEngine {
public:
    ExecutionEngine();
    ~ExecutionEngine();

    // Initialize with exchange adapters
    void initialize(const std::vector<std::shared_ptr<IExchangeAdapter>>& adapters);
    
    // Submit an order
    std::string submitOrder(const OrderRequest& request);
    
    // Cancel an order
    bool cancelOrder(const std::string& orderId);
    
    // Modify an order
    bool modifyOrder(const std::string& orderId, double newPrice, double newSize);
    
    // Register callback for execution reports
    void registerExecutionCallback(
        std::function<void(const OrderResponse&)> callback);

private:
    // Internal thread function for order processing
    void orderProcessingThread();
    
    // Exchange adapters
    std::vector<std::shared_ptr<IExchangeAdapter>> adapters_;
    
    // Order tracking
    struct OrderState {
        OrderRequest request;
        std::string exchangeOrderId;
        std::string status;
        double filledAmount;
        double averageFillPrice;
    };
    std::unordered_map<std::string, OrderState> orderStates_;  // clientOrderId -> state
    
    // Thread-safe queue for order requests
    struct OrderQueue {
        std::queue<OrderRequest> queue;
        std::mutex mutex;
        std::condition_variable cv;
    };
    std::unique_ptr<OrderQueue> orderQueue_;
    
    // Thread management
    std::thread orderProcessingThread_;
    std::atomic<bool> running_;
    
    // Execution callback
    std::function<void(const OrderResponse&)> executionCallback_;
    
    // Synchronization
    std::mutex orderStatesMutex_;
};

} // namespace crypto_hft 