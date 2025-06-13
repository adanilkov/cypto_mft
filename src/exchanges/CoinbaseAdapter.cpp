#include "CoinbaseAdapter.hpp"
#include "utils.hpp"
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <ctime>
#include <jwt-cpp/jwt.h>

namespace crypto_hft {

CoinbaseAdapter::CoinbaseAdapter()
    : ctx_(ssl::context::tls_client)
    , work_guard_(ioc_.get_executor())
    , strand_(net::make_strand(ioc_))
{
    initialize_logger();
    // load_config(config_path);
    // validate_config();
    
    // SSL context
    ctx_.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::no_sslv3 | ssl::context::single_dh_use);
    ctx_.set_verify_mode(ssl::verify_peer);
    ctx_.set_default_verify_paths();
    
    ws_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(strand_, ctx_);
    
    host_ = "advanced-trade-ws.coinbase.com";
    SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), host_.c_str());
    
    // Start IO thread
    io_thread_ = std::thread([this]() {
        try {
            ioc_.run();
        } catch (const std::exception& e) {
            logger_->error("Error in IO thread: {}", e.what());
        }
    });
}

CoinbaseAdapter::~CoinbaseAdapter() {
    disconnect();
    work_guard_.reset();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

bool CoinbaseAdapter::connect() {
    try {
        if (connected_) {
            logger_->warn("Already connected");
            return true;
        }

        // Resolve the host
        tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host_, "443");
        auto& lowest_layer = beast::get_lowest_layer(*ws_);
        lowest_layer.connect(results.begin()->endpoint());

        // SSL handshake
        ws_->next_layer().handshake(ssl::stream_base::client);

        // WebSocket handshake
        ws_->handshake(host_, "/");

        // Authenticate with Coinbase
        try {
            std::string jwt_token = crypto_hft::utils::coinbase_create_jwt();
            json auth_msg = {
                {"type", "authenticate"},
                {"token", jwt_token}
            };
            
            // Send authentication message
            do_write(auth_msg.dump());
            logger_->info("Sent authentication request to Coinbase");
            
            // Note: The actual authentication success/failure will be handled in on_message
            // when we receive the authenticate response
        } catch (const std::exception& e) {
            logger_->error("Failed to authenticate: {}", e.what());
            disconnect();
            return false;
        }

        connected_ = true;
        logger_->info("Successfully connected to Coinbase WebSocket");

        // Start reading
        do_read(); 
        
        return true;
    } catch (const std::exception& e) {
        logger_->error("Failed to connect: {}", e.what());
        return false;
    }
}

void CoinbaseAdapter::disconnect() {
    if (!connected_) {
        return;
    }

    try {
        connected_ = false;

        if (ws_) {
            ws_->async_close(websocket::close_code::normal,
                [this](beast::error_code ec) {
                    if (ec) {
                        logger_->error("Error during close: {}", ec.message());
                    }
                });
        }

        ioc_.stop();

        if (io_thread_.joinable()) {
            io_thread_.join();
        }

        logger_->info("Disconnected from Coinbase WebSocket");
    } catch (const std::exception& e) {
        logger_->error("Error during disconnect: {}", e.what());
    }
}

bool CoinbaseAdapter::isConnected() const {
    return connected_;
}

bool CoinbaseAdapter::subscribe(const std::vector<std::string>& symbols) {
    if (!connected_) {
        logger_->error("Cannot subscribe: WebSocket is not connected");
        return false;
    }

    try {
        // Create subscription message with correct format
        json subscribe_msg = {
            {"type", "subscribe"},
            {"product_ids", symbols},
            {"channel", "level2"}
        };

        // Set WebSocket to use text frames
        ws_->text(true);

        // Send the subscription message
        std::string message = subscribe_msg.dump();
        logger_->info("Sending subscription message: {}", message);
        
        do_write(message);
        logger_->info("Subscribed to symbols: {}", json(symbols).dump());
        return true;
    } catch (const std::exception& e) { 
        logger_->error("Failed to subscribe: {}", e.what());
        return false;
    }
}

bool CoinbaseAdapter::unsubscribe(const std::vector<std::string>& symbols) {
    if (!connected_) {
        logger_->error("Not connected");
        return false;
    }

    try {
        json unsubscribe_msg = {
            {"type", "unsubscribe"},
            {"channels", json::array({
                json{
                    {"name", "level2"},
                    {"product_ids", symbols}
                }
            })}
        };

        do_write(unsubscribe_msg.dump());
        logger_->info("Unsubscribed from symbols: {}", json(symbols).dump());
        return true;
    } catch (const std::exception& e) {
        logger_->error("Failed to unsubscribe: {}", e.what());
        return false;
    }
}

void CoinbaseAdapter::do_read() {
    ws_->async_read(
        buffer_,
        [this](beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
            on_message(ec, bytes_transferred);
        });
}

void CoinbaseAdapter::do_write(const std::string& message) {
    ws_->async_write(
        net::buffer(message),
        [this](beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                logger_->error("Write error: {}", ec.message());
            }
        });
}

void CoinbaseAdapter::on_message(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
    if (ec) {
        logger_->error("Read error: {}", ec.message());
        if (connected_) {
            // Try to reconnect if we're still supposed to be connected
            disconnect();
            connect();
        }
        return;
    }

    try {
        std::string message = beast::buffers_to_string(buffer_.data());
        logger_->debug("Received message: {}", message);
        buffer_.consume(buffer_.size());

        json data = json::parse(message);
        
        // Handle different message types
        if (data.contains("type")) {
            std::string type = data["type"];
            
            if (type == "error") {
                logger_->error("Received error message: {}", message);
                if (data.contains("message")) {
                    logger_->error("Error message: {}", data["message"].get<std::string>());
                }
                if (data.contains("error")) {
                    logger_->error("Error type: {}", data["error"].get<std::string>());
                }
                if (data.contains("details")) {
                    logger_->error("Error details: {}", data["details"].get<std::string>());
                }
                // If authentication failed, disconnect
                if (data.contains("error") && data["error"].get<std::string>() == "authentication_failed") {
                    disconnect();
                }
                return;
            }
            
            if (type == "authenticate") {
                if (data.contains("success") && data["success"].get<bool>()) {
                    logger_->info("Successfully authenticated with Coinbase");
                    authenticated_ = true;
                } else {
                    logger_->error("Authentication failed: {}", message);
                    authenticated_ = false;
                    disconnect();
                }
                return;
            }
            
            if (type == "subscriptions") {
                logger_->info("Received subscription confirmation: {}", message);
                return;
            }
            
            if (type == "snapshot") {
                handle_snapshot_message(data);
                return;
            }
            
            if (type == "l2update") {
                handle_l2update_message(data);
                return;
            }
            
            logger_->warn("Unknown message type: {}", type);
        }
    } catch (const std::exception& e) {
        logger_->error("Error processing message: {}", e.what());
    }

    // Continue reading
    do_read();
}

void CoinbaseAdapter::initialize_logger() {
    logger_ = spdlog::stdout_color_mt("coinbase_adapter");
    logger_->set_level(spdlog::level::debug);
}

void CoinbaseAdapter::load_config(const std::string& config_path) {
    try {
        config_ = YAML::LoadFile(config_path);
        base_url_ = "ws-feed.exchange.coinbase.com";  // Public feed URL
        // Load API credentials if present
        if (config_["api_key"]) {
            api_key_ = config_["api_key"].as<std::string>();
        }
        if (config_["api_secret"]) {
            api_secret_ = config_["api_secret"].as<std::string>();
        }
    } catch (const std::exception& e) {
        logger_->error("Failed to load config: {}", e.what());
        throw;
    }
}

void CoinbaseAdapter::handle_websocket_message(const std::string& message) {
    try {
        json data = json::parse(message);
        logger_->debug("Received message: {}", message);
        
        // Handle different message types
        if (data.contains("type")) {
            std::string type = data["type"];
            
            if (type == "ticker") {
                handle_ticker_message(data);
            } else if (type == "snapshot") {
                handle_snapshot_message(data);
            } else if (type == "l2update") {
                handle_l2update_message(data);
            } else if (type == "heartbeat") {
                handle_heartbeat_message(data);
            } else if (type == "error") {
                handle_error_message(data);
            } else if (type == "authenticate") {
                if (data.contains("message")) {
                    logger_->info("Authentication response: {}", data["message"].get<std::string>());
                }
                if (data.contains("error")) {
                    logger_->error("Authentication error: {}", data["error"].get<std::string>());
                }
            } else {
                logger_->warn("Unknown message type: {}", type);
            }
        } else {
            logger_->warn("Message without type field: {}", message);
        }
    } catch (const std::exception& e) {
        logger_->error("Error parsing message: {} - Raw message: {}", e.what(), message);
    }
}

// void CoinbaseAdapter::handle_ticker_message(const json& data) {
//     try {
//         MarketData ticker;
//         ticker.symbol = data["product_id"];
//         ticker.bidPrice = data.contains("best_bid") ? std::stod(data["best_bid"].get<std::string>()) : 0.0;
//         ticker.bidSize = data.contains("best_bid_size") ? std::stod(data["best_bid_size"].get<std::string>()) : 0.0;
//         ticker.askPrice = data.contains("best_ask") ? std::stod(data["best_ask"].get<std::string>()) : 0.0;
//         ticker.askSize = data.contains("best_ask_size") ? std::stod(data["best_ask_size"].get<std::string>()) : 0.0;
//         ticker.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
//             std::chrono::system_clock::now().time_since_epoch()
//         ).count();
        
//         if (market_data_handler_) {
//             market_data_handler_(ticker);
//         }
//     } catch (const std::exception& e) {
//         logger_->error("Error handling ticker message: {}", e.what());
//     }
// }
void CoinbaseAdapter::handle_ticker_message(const json& data) {
    logger_->debug("Received ticker message: {}", data.dump());
}

void CoinbaseAdapter::handle_snapshot_message(const json& data) {
    try {
        OrderBookSnapshot snapshot;
        snapshot.symbol = data["product_id"];
        snapshot.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        // Parse bids
        for (const auto& bid : data["bids"]) {
            double price = std::stod(bid[0].get<std::string>());
            double size = std::stod(bid[1].get<std::string>());
            snapshot.bids.push_back({price, size});
        }
        
        // Parse asks
        for (const auto& ask : data["asks"]) {
            double price = std::stod(ask[0].get<std::string>());
            double size = std::stod(ask[1].get<std::string>());
            snapshot.asks.push_back({price, size});
        }
        
        if (order_book_callback_) {
            order_book_callback_(snapshot);
        }
    } catch (const std::exception& e) {
        logger_->error("Error handling snapshot message: {}", e.what());
    }
}

void CoinbaseAdapter::handle_l2update_message(const json& data) {
    try {
        OrderBookDelta delta;
        delta.symbol = data["product_id"];
        delta.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        // Parse changes
        for (const auto& change : data["changes"]) {
            double price = std::stod(change[1].get<std::string>());
            double size = std::stod(change[2].get<std::string>());
            
            if (change[0] == "buy") {
                delta.bidUpdates.push_back({price, size});
            } else if (change[0] == "sell") {
                delta.askUpdates.push_back({price, size});
            }
        }
        
        if (order_book_delta_callback_) {
            order_book_delta_callback_(delta);
        }
    } catch (const std::exception& e) {
        logger_->error("Error handling l2update message: {}", e.what());
    }
}

void CoinbaseAdapter::handle_heartbeat_message(const json& data) {
    try {
        logger_->debug("Received heartbeat for product: {}", data["product_id"].get<std::string>());
    } catch (const std::exception& e) {
        logger_->error("Error handling heartbeat message: {}", e.what());
    }
}

void CoinbaseAdapter::handle_error_message(const json& data) {
    try {
        std::string error_message = "Unknown error";
        if (data.contains("message")) {
            error_message = data["message"].get<std::string>();
        }
        if (data.contains("error")) {
            error_message = data["error"].get<std::string>();
        }
        if (data.contains("details")) {
            error_message += " - Details: " + data["details"].dump();
        }
        logger_->error("Received error from Coinbase: {}", error_message);
    } catch (const std::exception& e) {
        logger_->error("Error handling error message: {}", e.what());
    }
}

void CoinbaseAdapter::registerOrderBookCallback(std::function<void(const OrderBookSnapshot&)> callback) {
    order_book_callback_ = std::move(callback);
}

void CoinbaseAdapter::registerOrderBookDeltaCallback(std::function<void(const OrderBookDelta&)> callback) {
    order_book_delta_callback_ = callback;
}

void CoinbaseAdapter::registerExecutionCallback(std::function<void(const OrderResponse&)> callback) {
    execution_callback_ = callback;
}

// Implement other interface methods with default values
std::string CoinbaseAdapter::getName() const { return "coinbase"; }
bool CoinbaseAdapter::supportsMargin() const { return false; }
double CoinbaseAdapter::getFeeRate(const std::string&) const { return 0.005; } // 0.5% default fee
double CoinbaseAdapter::getBalance(const std::string&) const { return 0.0; }
std::vector<std::pair<std::string, double>> CoinbaseAdapter::getAllBalances() const { return {}; }
std::string CoinbaseAdapter::submitOrder(const OrderRequest&) { return ""; }
bool CoinbaseAdapter::cancelOrder(const std::string&) { return false; }
bool CoinbaseAdapter::modifyOrder(const std::string&, double, double) { return false; }
bool CoinbaseAdapter::requestOrderBookSnapshot(const std::string&) { return false; }

} // namespace crypto_hft 