#pragma once

#include "IExchangeAdapter.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <functional>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <yaml-cpp/yaml.h>

using json = nlohmann::json;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

namespace crypto_hft {

// Forward declarations
struct OrderBookEntry {
    double price;
    double size;
};

// Message handler types
using MarketDataHandler = std::function<void(const MarketData&)>;
using OrderBookSnapshotHandler = std::function<void(const OrderBookSnapshot&)>;
using OrderBookDeltaHandler = std::function<void(const OrderBookDelta&)>;
using ExecutionHandler = std::function<void(const OrderResponse&)>;

class KrakenAdapter : public IExchangeAdapter {
public:
    KrakenAdapter();
    ~KrakenAdapter() override;

    // Prevent copying
    KrakenAdapter(const KrakenAdapter&) = delete;
    KrakenAdapter& operator=(const KrakenAdapter&) = delete;

    // IExchangeAdapter interface implementation
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    bool subscribe(const std::vector<std::string>& symbols) override;
    bool unsubscribe(const std::vector<std::string>& symbols) override;
    void set_message_handler(MarketDataHandler handler) { market_data_handler_ = handler; }

    // IExchangeAdapter interface implementation
    std::string getName() const override;
    bool supportsMargin() const override;
    double getFeeRate(const std::string& symbol) const override;
    double getBalance(const std::string& currency) const override;
    std::vector<std::pair<std::string, double>> getAllBalances() const override;
    std::string submitOrder(const OrderRequest& request) override;
    bool cancelOrder(const std::string& order_id) override;
    bool modifyOrder(const std::string& order_id, double new_price, double new_size) override;
    bool requestOrderBookSnapshot(const std::string& symbol) override;
    void registerOrderBookCallback(OrderBookSnapshotHandler callback) override;
    void registerOrderBookDeltaCallback(OrderBookDeltaHandler callback) override;
    void registerExecutionCallback(ExecutionHandler callback) override;

private:
    void on_message(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred);
    void on_connect(beast::error_code ec);
    void on_close(beast::error_code ec);
    void on_error(beast::error_code ec);
    void handle_websocket_message(const std::string& message);
    void handle_ticker_message(const json& data);
    void handle_snapshot_message(const json& data);
    void handle_l2update_message(const json& data);
    void handle_heartbeat_message(const json& data);
    void handle_error_message(const json& data);
    void initialize_logger();
    void load_config(const std::string& config_path);
    void do_read();
    void do_write(const std::string& message);

    // WebSocket client
    net::io_context ioc_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    net::strand<net::io_context::executor_type> strand_;
    ssl::context ctx_{ssl::context::tlsv12_client};
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_;
    
    std::thread io_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::string> message_queue_;
    MarketDataHandler market_data_handler_;
    OrderBookSnapshotHandler order_book_callback_;
    OrderBookDeltaHandler order_book_delta_callback_;
    ExecutionHandler execution_callback_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::string api_key_;
    std::string api_secret_;
    std::string base_url_;
    YAML::Node config_;
    std::shared_ptr<spdlog::logger> logger_;
    beast::flat_buffer buffer_;
    std::string host_;  // Store the WebSocket host name
    std::unordered_map<std::string, std::string> ws_subscriptions_;
};

} // namespace crypto_hft 