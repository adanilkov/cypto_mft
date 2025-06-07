#pragma once

#include <string>
#include <chrono>
#include <cstdint>

namespace crypto_hft {
namespace utils {

// Order side (buy/sell)
enum class OrderSide {
    BUY,
    SELL
};

// Order type (market/limit)
enum class OrderType {
    MARKET,
    LIMIT
};

// Order status
enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELED,
    REJECTED
};

// Order request structure
struct OrderRequest {
    std::string symbol;
    OrderSide side;
    OrderType type;
    double quantity;
    double price;  // Only used for limit orders
    std::string clientOrderId;
    std::chrono::system_clock::time_point timestamp;
};

// Order cancel request structure
struct OrderCancelRequest {
    std::string symbol;
    std::string orderId;
    std::string clientOrderId;
    std::chrono::system_clock::time_point timestamp;
};

// Market data update structure
struct MarketUpdate {
    std::string symbol;
    double bidPrice;
    double bidQuantity;
    double askPrice;
    double askQuantity;
    std::chrono::system_clock::time_point timestamp;
};

// Execution report structure
struct ExecReport {
    std::string symbol;
    std::string orderId;
    std::string clientOrderId;
    OrderStatus status;
    OrderSide side;
    OrderType type;
    double executedQuantity;
    double remainingQuantity;
    double averagePrice;
    std::chrono::system_clock::time_point timestamp;
};

} // namespace utils
} // namespace crypto_hft 