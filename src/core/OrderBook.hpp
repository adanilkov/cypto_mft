#pragma once

#include <map>
#include <atomic>
#include <memory>
#include <string>
#include <shared_mutex>

namespace crypto_hft {

struct OrderBookLevel {
    double price;
    double size;
};

class OrderBook {
public:
    OrderBook(const std::string& symbol);
    
    double getBestBid() const;
    double getBestAsk() const;
    double getSpread() const;
    double getMidPrice() const;
    
    double getBidVolume(double price) const;
    double getAskVolume(double price) const;
    
    double getCumulativeBidVolume(double price) const;
    double getCumulativeAskVolume(double price) const;
    
    void updateBid(double price, double size);
    void updateAsk(double price, double size);
    void removeBid(double price);
    void removeAsk(double price);
    
    void clear();
    void setSnapshot(const std::map<double, double>& bids, 
                    const std::map<double, double>& asks);
    
    std::map<double, double> getBids() const;
    std::map<double, double> getAsks() const;

private:
    std::string symbol_;
    
    mutable std::shared_mutex m;
    std::map<double, double, std::greater<double>> bids;
    std::map<double, double> asks;
    
    // void atomicUpdate(std::atomic<std::shared_ptr<std::map<double, double, std::greater<double>>>>& book,
    //                  double price, double size, bool isRemove);
    // void atomicUpdate(std::atomic<std::shared_ptr<std::map<double, double>>>& book,
    //                  double price, double size, bool isRemove);
};
} // namespace crypto_hft 