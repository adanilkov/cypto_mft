#include "MockExchangeAdapter.hpp"
#include <fstream>
#include <random>
#include <spdlog/spdlog.h>

namespace crypto_hft {

MockExchangeAdapter::MockExchangeAdapter()
    : connected_(false)
    , running_(false)
    , simulated_latency_(std::chrono::milliseconds(50)) {
}

MockExchangeAdapter::~MockExchangeAdapter() {
    disconnect();
}

bool MockExchangeAdapter::connect() {
    if (connected_) {
        return true;
    }
    connected_ = true;
    return true;
}

void MockExchangeAdapter::disconnect() {
    if (!connected_) {
        return;
    }
    stopMarketDataSimulation();
    connected_ = false;
}

bool MockExchangeAdapter::isConnected() const {
    return connected_;
}

bool MockExchangeAdapter::subscribe([[maybe_unused]] const std::vector<std::string>& symbols) {
    if (!connected_) {
        return false;
    }
    return true;
}

bool MockExchangeAdapter::unsubscribe([[maybe_unused]] const std::vector<std::string>& symbols) {
    if (!connected_) {
        return false;
    }
    return true;
}

bool MockExchangeAdapter::requestOrderBookSnapshot([[maybe_unused]] const std::string& symbol) {
    if (!connected_ || !order_book_callback_) {
        return false;
    }
    return true;
}

void MockExchangeAdapter::registerOrderBookCallback(
    std::function<void(const OrderBookSnapshot&)> callback) {
    order_book_callback_ = std::move(callback);
}

void MockExchangeAdapter::registerOrderBookDeltaCallback(
    std::function<void(const OrderBookDelta&)> callback) {
    order_book_delta_callback_ = std::move(callback);
}

std::string MockExchangeAdapter::submitOrder(const OrderRequest& request) {
    if (!connected_) {
        return "";
    }

    // Generate a unique order ID
    static std::atomic<uint64_t> order_counter{0};
    std::string orderId = "MOCK_" + std::to_string(++order_counter);

    // Store order state
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        orders_[orderId] = OrderState{
            request,
            "NEW",
            0.0,
            0.0
        };
    }

    // Simulate order execution in a separate thread
    std::thread([this, orderId]() {
        simulateOrderExecution(orderId);
    }).detach();

    return orderId;
}

bool MockExchangeAdapter::cancelOrder(const std::string& orderId) {
    if (!connected_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        return false;
    }

    // Only cancel if not already filled
    if (it->second.status != "FILLED") {
        it->second.status = "CANCELED";
        if (execution_callback_) {
            OrderResponse response{
                orderId,
                it->second.request.clientOrderId,
                "CANCELED",
                it->second.filled_amount,
                it->second.fill_price,
                std::chrono::system_clock::now().time_since_epoch().count()
            };
            execution_callback_(response);
        }
        return true;
    }
    return false;
}

bool MockExchangeAdapter::modifyOrder(const std::string& orderId,
                                    double newPrice,
                                    double newSize) {
    if (!connected_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        return false;
    }

    it->second.request.price = newPrice;
    it->second.request.size = newSize;
    return true;
}

void MockExchangeAdapter::registerExecutionCallback(
    std::function<void(const OrderResponse&)> callback) {
    execution_callback_ = std::move(callback);
}

double MockExchangeAdapter::getBalance([[maybe_unused]] const std::string& asset) const {
    // Mock implementation returns a fixed balance
    return 1000.0;
}

std::vector<std::pair<std::string, double>> MockExchangeAdapter::getAllBalances() const {
    // Mock implementation returns fixed balances for common assets
    return {
        {"BTC", 1.0},
        {"ETH", 10.0},
        {"USDT", 10000.0}
    };
}

std::string MockExchangeAdapter::getName() const {
    return "MockExchange";
}

bool MockExchangeAdapter::supportsMargin() const {
    return true;
}

double MockExchangeAdapter::getFeeRate([[maybe_unused]] const std::string& symbol) const {
    // Mock implementation returns a fixed fee rate
    return 0.001;  // 0.1%
}

void MockExchangeAdapter::feedMarketData(const MarketData& data) {
    if (!connected_ || !order_book_delta_callback_) {
        return;
    }

    OrderBookDelta delta{
        data.symbol,
        {{data.bidPrice, data.bidSize}},
        {{data.askPrice, data.askSize}},
        data.timestamp
    };

    order_book_delta_callback_(delta);
}

void MockExchangeAdapter::feedOrderBookSnapshot(const OrderBookSnapshot& snapshot) {
    if (!connected_ || !order_book_callback_) {
        return;
    }
    order_book_callback_(snapshot);
}

void MockExchangeAdapter::feedOrderBookDelta(const OrderBookDelta& delta) {
    if (!connected_ || !order_book_delta_callback_) {
        return;
    }
    order_book_delta_callback_(delta);
}

void MockExchangeAdapter::startMarketDataSimulation() {
    if (!connected_ || running_) {
        return;
    }

    running_ = true;
    simulation_thread_ = std::thread([this]() {
        simulateMarketData();
    });
}

void MockExchangeAdapter::stopMarketDataSimulation() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (simulation_thread_.joinable()) {
        simulation_thread_.join();
    }
}

void MockExchangeAdapter::setSimulatedLatency(std::chrono::milliseconds latency) {
    simulated_latency_ = latency;
}

void MockExchangeAdapter::simulateMarketData() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(10000.0, 11000.0);
    std::uniform_real_distribution<> size_dist(0.1, 1.0);

    while (running_) {
        MarketData data{
            "BTC/USD",  // Changed from BTC-USD to BTC/USD
            price_dist(gen),
            size_dist(gen),
            price_dist(gen),
            size_dist(gen),
            std::chrono::system_clock::now().time_since_epoch().count()
        };

        feedMarketData(data);
        std::this_thread::sleep_for(simulated_latency_);
    }
}

void MockExchangeAdapter::simulateOrderExecution(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        return;
    }

    // Send NEW status immediately
    if (execution_callback_) {
        OrderResponse response{
            orderId,
            it->second.request.clientOrderId,
            "NEW",
            0.0,
            0.0,
            std::chrono::system_clock::now().time_since_epoch().count()
        };
        execution_callback_(response);
    }

    // Add a small delay to allow for cancellation
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Check if order was cancelled before filling
    if (it->second.status == "CANCELED") {
        if (execution_callback_) {
            OrderResponse response{
                orderId,
                it->second.request.clientOrderId,
                "CANCELED",
                0.0,
                0.0,
                std::chrono::system_clock::now().time_since_epoch().count()
            };
            execution_callback_(response);
        }
        return;
    }

    // Simulate fill
    double fill_amount = it->second.request.size;
    double fill_price = it->second.request.price;
    
    it->second.filled_amount = fill_amount;
    it->second.fill_price = fill_price;
    it->second.status = "FILLED";
    
    if (execution_callback_) {
        OrderResponse response{
            orderId,
            it->second.request.clientOrderId,
            "FILLED",
            fill_amount,
            fill_price,
            std::chrono::system_clock::now().time_since_epoch().count()
        };
        execution_callback_(response);
    }
}

} // namespace crypto_hft 