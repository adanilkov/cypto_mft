#include "KrakenAdapter.hpp"
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

KrakenAdapter::KrakenAdapter()
    : ctx_(ssl::context::tls_client)
    , work_guard_(ioc_.get_executor())
    , strand_(net::make_strand(ioc_))
{
    initialize_logger();
    
    // SSL context
    ctx_.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::no_sslv3 | ssl::context::single_dh_use);
    ctx_.set_verify_mode(ssl::verify_peer);
    ctx_.set_default_verify_paths();
    
    ws_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(strand_, ctx_);
    
    host_ = "ws.kraken.com";
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

KrakenAdapter::~KrakenAdapter() {
    disconnect();
    work_guard_.reset();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

bool KrakenAdapter::connect() {
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

        connected_ = true;
        logger_->info("Successfully connected to Kraken WebSocket");

        // Start reading
        do_read();
        
        return true;
    } catch (const std::exception& e) {
        logger_->error("Failed to connect: {}", e.what());
        return false;
    }
}

void KrakenAdapter::disconnect() {
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

        logger_->info("Disconnected from Kraken WebSocket");
    } catch (const std::exception& e) {
        logger_->error("Error during disconnect: {}", e.what());
    }
}

bool KrakenAdapter::isConnected() const {
    return connected_;
}

bool KrakenAdapter::subscribe(const std::vector<std::string>& symbols) {
    if (!connected_) {
        logger_->error("Cannot subscribe: WebSocket is not connected");
        return false;
    }

    try {
        // Create subscription message with correct format for Kraken
        json subscribe_msg = {
            {"event", "subscribe"},
            {"pair", symbols},
            {"subscription", {
                {"name", "book"}
            }}
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

bool KrakenAdapter::unsubscribe(const std::vector<std::string>& symbols) {
    if (!connected_) {
        logger_->error("Not connected");
        return false;
    }

    try {
        json unsubscribe_msg = {
            {"event", "unsubscribe"},
            {"pair", symbols}
        };

        do_write(unsubscribe_msg.dump());
        logger_->info("Unsubscribed from symbols: {}", json(symbols).dump());
        return true;
    } catch (const std::exception& e) {
        logger_->error("Failed to unsubscribe: {}", e.what());
        return false;
    }
}

void KrakenAdapter::do_read() {
    ws_->async_read(
        buffer_,
        [this](beast::error_code ec, std::size_t bytes_transferred) {
            on_message(ec, bytes_transferred);
        });
}

void KrakenAdapter::do_write(const std::string& message) {
    ws_->async_write(
        net::buffer(message),
        [this](beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                logger_->error("Write error: {}", ec.message());
            }
        });
}

void KrakenAdapter::on_message(beast::error_code ec, std::size_t bytes_transferred) {
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
        if (data.contains("event")) {
            std::string event = data["event"];
            
            if (event == "systemStatus") {
                logger_->info("System status: {}", message);
                return;
            }
            
            if (event == "subscriptionStatus") {
                logger_->info("Subscription status: {}", message);
                return;
            }
            
            if (event == "error") {
                logger_->error("Received error message: {}", message);
                return;
            }
        }
        
        // Handle order book updates
        if (data.is_array() && data.size() >= 2) {
            // First element is channel ID, second is the actual data
            const auto& book_data = data[1];
            
            if (book_data.contains("bs") || book_data.contains("as")) {
                // This is a snapshot
                handle_snapshot_message(book_data);
            } else if (book_data.contains("b") || book_data.contains("a")) {
                // This is an update
                handle_l2update_message(book_data);
            }
        }
    } catch (const std::exception& e) {
        logger_->error("Error processing message: {}", e.what());
    }

    // Continue reading
    do_read();
}

void KrakenAdapter::initialize_logger() {
    logger_ = spdlog::stdout_color_mt("kraken_adapter");
    logger_->set_level(spdlog::level::debug);
}

void KrakenAdapter::handle_snapshot_message(const json& data) {
    try {
        OrderBookSnapshot snapshot;
        // Extract symbol from the subscription
        snapshot.symbol = "BTC/USD"; // TODO: Map from subscription ID
        snapshot.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        // Process bids
        if (data.contains("bs")) {
            for (const auto& bid : data["bs"]) {
                double price = std::stod(bid[0].get<std::string>());
                double size = std::stod(bid[1].get<std::string>());
                snapshot.bids.push_back({price, size});
            }
        }
        
        // Process asks
        if (data.contains("as")) {
            for (const auto& ask : data["as"]) {
                double price = std::stod(ask[0].get<std::string>());
                double size = std::stod(ask[1].get<std::string>());
                snapshot.asks.push_back({price, size});
            }
        }
        
        if (order_book_callback_) {
            order_book_callback_(snapshot);
        }
    } catch (const std::exception& e) {
        logger_->error("Error handling snapshot message: {}", e.what());
    }
}

void KrakenAdapter::handle_l2update_message(const json& data) {
    try {
        OrderBookDelta delta;
        // Extract symbol from the subscription
        delta.symbol = "BTC/USD"; // TODO: Map from subscription ID
        delta.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        // Process bid updates
        if (data.contains("b")) {
            for (const auto& bid : data["b"]) {
                double price = std::stod(bid[0].get<std::string>());
                double size = std::stod(bid[1].get<std::string>());
                delta.bidUpdates.push_back({price, size});
            }
        }
        
        // Process ask updates
        if (data.contains("a")) {
            for (const auto& ask : data["a"]) {
                double price = std::stod(ask[0].get<std::string>());
                double size = std::stod(ask[1].get<std::string>());
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

// Implement other required interface methods
std::string KrakenAdapter::getName() const { return "Kraken"; }
bool KrakenAdapter::supportsMargin() const { return true; }
double KrakenAdapter::getFeeRate(const std::string& symbol) const { return 0.0026; } // 0.26% maker/taker fee
double KrakenAdapter::getBalance(const std::string& asset) const { return 0.0; }
std::vector<std::pair<std::string, double>> KrakenAdapter::getAllBalances() const { return {}; }
std::string KrakenAdapter::submitOrder(const OrderRequest& request) { return ""; }
bool KrakenAdapter::cancelOrder(const std::string& orderId) { return false; }
bool KrakenAdapter::modifyOrder(const std::string& orderId, double newPrice, double newSize) { return false; }
bool KrakenAdapter::requestOrderBookSnapshot(const std::string& symbol) { return false; }

void KrakenAdapter::registerOrderBookCallback(std::function<void(const OrderBookSnapshot&)> callback) {
    order_book_callback_ = std::move(callback);
}

void KrakenAdapter::registerOrderBookDeltaCallback(std::function<void(const OrderBookDelta&)> callback) {
    order_book_delta_callback_ = std::move(callback);
}

void KrakenAdapter::registerExecutionCallback(std::function<void(const OrderResponse&)> callback) {
    execution_callback_ = std::move(callback);
}

} // namespace crypto_hft 