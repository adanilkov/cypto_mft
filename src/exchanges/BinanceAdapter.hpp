#pragma once

#include "IExchangeAdapter.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <thread>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

namespace crypto_hft {

// Forward declarations
struct OrderBookEntry;
struct OrderBookSnapshot;
struct OrderBookDelta;
struct OrderResponse;
struct OrderRequest;

// Type aliases for handlers
using OrderBookSnapshotHandler = std::function<void(const OrderBookSnapshot&)>;
using OrderBookDeltaHandler = std::function<void(const OrderBookDelta&)>;
using ExecutionHandler = std::function<void(const OrderResponse&)>;

class BinanceAdapter : public IExchangeAdapter {
public:
    BinanceAdapter();
    ~BinanceAdapter() override;

    // Connection management
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    // Subscription management
    bool subscribe(const std::vector<std::string>& symbols) override;
    bool unsubscribe(const std::vector<std::string>& symbols) override;

    // Exchange information
    std::string getName() const override;
    bool supportsMargin() const override;
    double getFeeRate(const std::string& symbol) const override;

    // Account management
    double getBalance(const std::string& asset) const override;
    std::vector<std::pair<std::string, double>> getAllBalances() const override;

    // Order management
    std::string submitOrder(const OrderRequest& request) override;
    bool cancelOrder(const std::string& orderId) override;
    bool modifyOrder(const std::string& orderId, double newPrice, double newSize) override;
    bool requestOrderBookSnapshot(const std::string& symbol) override;

    // Callback registration
    void registerOrderBookCallback(std::function<void(const OrderBookSnapshot&)> callback) override;
    void registerOrderBookDeltaCallback(std::function<void(const OrderBookDelta&)> callback) override;
    void registerExecutionCallback(std::function<void(const OrderResponse&)> callback) override;

private:
    // WebSocket client
    ssl::context ctx_;
    net::io_context ioc_;
    net::executor_work_guard<net::io_context::executor_type> work_guard_;
    net::strand<net::io_context::executor_type> strand_;
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_;
    beast::flat_buffer buffer_;
    std::thread io_thread_;
    std::string host_;
    bool connected_ = false;

    // Callbacks
    std::function<void(const OrderBookSnapshot&)> order_book_callback_;
    std::function<void(const OrderBookDelta&)> order_book_delta_callback_;
    std::function<void(const OrderResponse&)> execution_callback_;

    // Logger
    std::shared_ptr<spdlog::logger> logger_;

    // Internal methods
    void initialize_logger();
    void do_read();
    void do_write(const std::string& message);
    void on_message(beast::error_code ec, std::size_t bytes_transferred);
    void handle_snapshot_message(const json& data);
    void handle_l2update_message(const json& data);
};

} // namespace crypto_hft 