#pragma once

#include "IExchangeAdapter.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

namespace crypto_hft {

class KrakenAdapter : public IExchangeAdapter {
public:
    KrakenAdapter(const std::string& config_path);
    ~KrakenAdapter() override;

    // Prevent copying
    KrakenAdapter(const KrakenAdapter&) = delete;
    KrakenAdapter& operator=(const KrakenAdapter&) = delete;

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
    using WebSocketClient = websocketpp::client<websocketpp::config::asio_tls_client>;
    std::unique_ptr<WebSocketClient> ws_client_;
    websocketpp::connection_hdl ws_connection_;
    
    // REST API endpoints
    std::string base_url_;
    std::string api_key_;
    std::string api_secret_;
    
    // WebSocket subscriptions
    std::unordered_map<std::string, std::string> ws_subscriptions_;
    
    void initialize_logger();
    void load_config(const std::string& config_path);
    void validate_config() const;
    void setup_websocket();
    void handle_websocket_message(websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg);
    std::string generate_signature(const std::string& nonce, const std::string& post_data) const;
    std::string make_request(const std::string& endpoint, const std::string& method, const std::string& params = "");
};

} // namespace crypto_hft 