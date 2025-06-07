#pragma once

#include <map>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <string>

namespace crypto_hft {

struct OrderBookLevel {
    double price;
    double size;
};

class OrderBook {
public:
    OrderBook(const std::string& symbol);
    
    // Thread-safe read operations
    double getBestBid() const;
    double getBestAsk() const;
    double getSpread() const;
    double getMidPrice() const;
    
    // Get volume at price level
    double getBidVolume(double price) const;
    double getAskVolume(double price) const;
    
    // Get cumulative volume up to price level
    double getCumulativeBidVolume(double price) const;
    double getCumulativeAskVolume(double price) const;
    
    // Thread-safe write operations (called by MarketDataEngine)
    void updateBid(double price, double size);
    void updateAsk(double price, double size);
    void removeBid(double price);
    void removeAsk(double price);
    
    // Snapshot operations
    void clear();
    void setSnapshot(const std::map<double, double>& bids, 
                    const std::map<double, double>& asks);
    
    // Get current depth
    std::map<double, double> getBids() const;
    std::map<double, double> getAsks() const;

private:
    std::string symbol_;
    
    // Price-time priority order books
    std::map<double, double, std::greater<double>> bids_;  // price -> size
    std::map<double, double> asks_;  // price -> size
    
    // Reader-writer lock for thread safety
    mutable std::shared_mutex mutex_;
};

} // namespace crypto_hft 