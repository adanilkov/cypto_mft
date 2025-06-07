#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace crypto_hft {

struct MarketData {
    std::string symbol;
    double bidPrice;
    double bidSize;
    double askPrice;
    double askSize;
    int64_t timestamp;
};

struct OrderBookSnapshot {
    std::string symbol;
    std::vector<std::pair<double, double>> bids;  // price, size
    std::vector<std::pair<double, double>> asks;  // price, size
    int64_t timestamp;
};

struct OrderBookDelta {
    std::string symbol;
    std::vector<std::pair<double, double>> bidUpdates;  // price, size (0 for removal)
    std::vector<std::pair<double, double>> askUpdates;  // price, size (0 for removal)
    int64_t timestamp;
};

struct OrderRequest {
    std::string symbol;
    enum class Side { BUY, SELL } side;
    enum class Type { MARKET, LIMIT } type;
    double price;  // For limit orders
    double size;
    std::string clientOrderId;
};

struct OrderResponse {
    std::string orderId;
    std::string clientOrderId;
    std::string status;  // NEW, PARTIALLY_FILLED, FILLED, CANCELED, REJECTED
    double filledAmount;
    double fillPrice;
    int64_t timestamp;
};

class IExchangeAdapter {
public:
    virtual ~IExchangeAdapter() = default;

    // Connection management
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // Market data subscription
    virtual bool subscribe(const std::vector<std::string>& symbols) = 0;
    virtual bool unsubscribe(const std::vector<std::string>& symbols) = 0;

    // Order book management
    virtual bool requestOrderBookSnapshot(const std::string& symbol) = 0;
    virtual void registerOrderBookCallback(
        std::function<void(const OrderBookSnapshot&)> callback) = 0;
    virtual void registerOrderBookDeltaCallback(
        std::function<void(const OrderBookDelta&)> callback) = 0;

    // Order management
    virtual std::string submitOrder(const OrderRequest& request) = 0;
    virtual bool cancelOrder(const std::string& orderId) = 0;
    virtual bool modifyOrder(const std::string& orderId,
                           double newPrice,
                           double newSize) = 0;

    // Execution reports
    virtual void registerExecutionCallback(
        std::function<void(const OrderResponse&)> callback) = 0;

    // Account information
    virtual double getBalance(const std::string& asset) const = 0;
    virtual std::vector<std::pair<std::string, double>> getAllBalances() const = 0;

    // Exchange information
    virtual std::string getName() const = 0;
    virtual bool supportsMargin() const = 0;
    virtual double getFeeRate(const std::string& symbol) const = 0;
};

} // namespace crypto_hft 