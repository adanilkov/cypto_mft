#pragma once

#include "IExchangeAdapter.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/system/error_code.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <yaml-cpp/yaml.h>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

namespace crypto_hft {

class CoinbaseAdapter : public IExchangeAdapter {
public:
    CoinbaseAdapter(const std::string& config_path);
    ~CoinbaseAdapter() override;

    // Prevent copying
    CoinbaseAdapter(const CoinbaseAdapter&) = delete;
    CoinbaseAdapter& operator=(const CoinbaseAdapter&) = delete;

    // IExchangeAdapter interface implementation
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    
    // Market data subscription
    bool subscribe(const std::vector<std::string>& symbols) override;
    bool unsubscribe(const std::vector<std::string>& symbols) override;
    
    // Order book management
    bool requestOrderBookSnapshot(const std::string& symbol) override;
    void registerOrderBookCallback(std::function<void(const OrderBookSnapshot&)> callback) override;
    void registerOrderBookDeltaCallback(std::function<void(const OrderBookDelta&)> callback) override;
    
    // Order management
    std::string submitOrder(const OrderRequest& request) override;
    bool cancelOrder(const std::string& orderId) override;
    bool modifyOrder(const std::string& orderId, double newPrice, double newSize) override;
    
    // Execution reports
    void registerExecutionCallback(std::function<void(const OrderResponse&)> callback) override;
    
    // Account information
    double getBalance(const std::string& asset) const override;
    std::vector<std::pair<std::string, double>> getAllBalances() const override;
    
    // Exchange information
    std::string getName() const override;
    bool supportsMargin() const override;
    double getFeeRate(const std::string& symbol) const override;

private:
    std::shared_ptr<spdlog::logger> logger_;
    YAML::Node config_;
    
    // WebSocket client
    net::io_context ioc_;
    ssl::context ssl_ctx_{ssl::context::tlsv12_client};
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_;
    bool connected_{false};
    
    // REST API endpoints
    std::string base_url_;
    std::string api_key_;
    std::string api_secret_;
    std::string passphrase_;
    
    // WebSocket channels
    std::unordered_map<std::string, std::string> ws_channels_;
    
    void initialize_logger();
    void load_config(const std::string& config_path);
    void validate_config() const;
    void setup_websocket();
    void handle_websocket_message(const std::string& message);
    std::string generate_signature(const std::string& timestamp, const std::string& method, 
                                 const std::string& request_path, const std::string& body) const;
    std::string make_request(const std::string& endpoint, const std::string& method, const std::string& body = "");
};

} // namespace crypto_hft 