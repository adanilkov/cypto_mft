#include "OrderBook.hpp"
#include <shared_mutex>
#include <mutex>

namespace crypto_hft {

OrderBook::OrderBook(const std::string& symbol) 
    : symbol_(symbol) {}

void OrderBook::updateBid(double price, double size) {
    std::unique_lock<std::shared_mutex> lock(m);
    if (asks.empty() || price < asks.begin()->first) {
        bids[price] = size;
    }
}

void OrderBook::updateAsk(double price, double size) {
    std::unique_lock<std::shared_mutex> lock(m);
    if (bids.empty() || price > bids.begin()->first) {
        asks[price] = size;
    }
}

void OrderBook::removeBid(double price) {
    std::unique_lock<std::shared_mutex> lock(m);
    bids.erase(price);
}

void OrderBook::removeAsk(double price) {
    std::unique_lock<std::shared_mutex> lock(m);
    asks.erase(price);
}

void OrderBook::setSnapshot(const std::map<double, double>& newBids, const std::map<double, double>& newAsks) {
    std::unique_lock<std::shared_mutex> lock(m);
    bids = std::map<double, double, std::greater<double>>(newBids.begin(), newBids.end());
    asks = newAsks;
}

void OrderBook::clear() {
    std::unique_lock<std::shared_mutex> lock(m);
    bids.clear();
    asks.clear();
}

double OrderBook::getBestBid() const {
    std::shared_lock<std::shared_mutex> lock(m);
    return bids.empty() ? 0.0 : bids.begin()->first;
}

double OrderBook::getBestAsk() const {
    std::shared_lock<std::shared_mutex> lock(m);
    return asks.empty() ? 0.0 : asks.begin()->first;
}

double OrderBook::getSpread() const {
    return getBestAsk() - getBestBid();
}

double OrderBook::getMidPrice() const {
    return (getBestBid() + getBestAsk()) / 2.0;
}

double OrderBook::getBidVolume(double price) const {
    std::shared_lock<std::shared_mutex> lock(m);
    auto it = bids.find(price);
    return it != bids.end() ? it->second : 0.0;
}

double OrderBook::getAskVolume(double price) const {
    std::shared_lock<std::shared_mutex> lock(m);
    auto it = asks.find(price);
    return it != asks.end() ? it->second : 0.0;
}

double OrderBook::getCumulativeBidVolume(double price) const {
    std::shared_lock<std::shared_mutex> lock(m);
    double volume = 0.0;
    for (const auto& [p, s] : bids) {
        if (p <= price) break;
        volume += s;
    }
    return volume;
}

double OrderBook::getCumulativeAskVolume(double price) const {
    std::shared_lock<std::shared_mutex> lock(m);
    double volume = 0.0;
    for (const auto& [p, s] : asks) {
        if (p >= price) break;
        volume += s;
    }
    return volume;
}

std::map<double, double> OrderBook::getBids() const {
    std::shared_lock<std::shared_mutex> lock(m);
    return std::map<double, double>(bids.begin(), bids.end());
}

std::map<double, double> OrderBook::getAsks() const {
    std::shared_lock<std::shared_mutex> lock(m);
    return asks;
}

} // namespace crypto_hft