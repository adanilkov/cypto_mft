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

bool MockExchangeAdapter::subscribe(const std::vector<std::string>& symbols) {
    if (!connected_) {
        return false;
    }
    return true;
}

bool MockExchangeAdapter::unsubscribe(const std::vector<std::string>& symbols) {
    if (!connected_) {
        return false;
    }
    return true;
}

bool MockExchangeAdapter::requestOrderBookSnapshot(const std::string& symbol) {
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

double MockExchangeAdapter::getBalance(const std::string& asset) const {
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

double MockExchangeAdapter::getFeeRate(const std::string& symbol) const {
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

void MockExchangeAdapter::setSimulatedLatency(std::chrono::milliseconds latency) {
    simulated_latency_ = latency;
}

void MockExchangeAdapter::loadMarketDataFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        spdlog::error("Failed to open market data file: {}", filename);
        return;
    }

    nlohmann::json data;
    file >> data;
    loadMarketDataFromJson(data);
}

void MockExchangeAdapter::startMarketDataSimulation() {
    if (running_) {
        return;
    }

    running_ = true;
    market_data_thread_ = std::thread(&MockExchangeAdapter::marketDataSimulationLoop, this);
}

void MockExchangeAdapter::stopMarketDataSimulation() {
    if (!running_) {
        return;
    }

    running_ = false;
    queue_cv_.notify_all();
    if (market_data_thread_.joinable()) {
        market_data_thread_.join();
    }
}

void MockExchangeAdapter::marketDataSimulationLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this]() {
            return !running_ || !market_data_queue_.empty();
        });

        if (!running_) {
            break;
        }

        if (market_data_queue_.empty()) {
            continue;
        }

        auto data = market_data_queue_.front();
        market_data_queue_.pop();
        lock.unlock();

        // Simulate market data processing
        std::this_thread::sleep_for(simulated_latency_);

        if (data.contains("type") && data["type"] == "snapshot") {
            OrderBookSnapshot snapshot;
            snapshot.symbol = data["symbol"];
            snapshot.timestamp = data["timestamp"];

            for (const auto& bid : data["bids"]) {
                snapshot.bids.emplace_back(bid[0], bid[1]);
            }
            for (const auto& ask : data["asks"]) {
                snapshot.asks.emplace_back(ask[0], ask[1]);
            }

            if (order_book_callback_) {
                order_book_callback_(snapshot);
            }
        } else if (data.contains("type") && data["type"] == "delta") {
            OrderBookDelta delta;
            delta.symbol = data["symbol"];
            delta.timestamp = data["timestamp"];

            for (const auto& bid : data["bidUpdates"]) {
                delta.bidUpdates.emplace_back(bid[0], bid[1]);
            }
            for (const auto& ask : data["askUpdates"]) {
                delta.askUpdates.emplace_back(ask[0], ask[1]);
            }

            if (order_book_delta_callback_) {
                order_book_delta_callback_(delta);
            }
        }
    }
}

void MockExchangeAdapter::simulateOrderExecution(const std::string& orderId) {
    std::this_thread::sleep_for(simulated_latency_);

    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        return;
    }

    // Simulate order acknowledgment
    if (execution_callback_) {
        OrderResponse ack{
            orderId,
            it->second.request.clientOrderId,
            "NEW",
            0.0,
            0.0,
            std::chrono::system_clock::now().time_since_epoch().count()
        };
        execution_callback_(ack);
    }

    // Simulate order fill after a short delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    it->second.status = "FILLED";
    it->second.filled_amount = it->second.request.size;
    it->second.fill_price = it->second.request.price;

    if (execution_callback_) {
        OrderResponse fill{
            orderId,
            it->second.request.clientOrderId,
            "FILLED",
            it->second.filled_amount,
            it->second.fill_price,
            std::chrono::system_clock::now().time_since_epoch().count()
        };
        execution_callback_(fill);
    }
}

void MockExchangeAdapter::loadMarketDataFromJson(const nlohmann::json& data) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (data.is_array()) {
        for (const auto& item : data) {
            market_data_queue_.push(item);
        }
    } else {
        market_data_queue_.push(data);
    }
    queue_cv_.notify_one();
}

} // namespace crypto_hft 