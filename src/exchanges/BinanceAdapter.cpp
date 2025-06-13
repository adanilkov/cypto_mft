#include "BinanceAdapter.hpp"
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
#include <spdlog/sinks/stdout_color_sinks.h>

namespace crypto_hft {

BinanceAdapter::BinanceAdapter()
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
    
    host_ = "stream.binance.com";
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

BinanceAdapter::~BinanceAdapter() {
    disconnect();
    work_guard_.reset();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

bool BinanceAdapter::connect() {
    try {
        if (connected_) {
            logger_->warn("Already connected");
            return true;
        }

        // Resolve the host
        tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host_, "9443");
        auto& lowest_layer = beast::get_lowest_layer(*ws_);
        lowest_layer.connect(results.begin()->endpoint());

        // SSL handshake
        ws_->next_layer().handshake(ssl::stream_base::client);

        // WebSocket handshake
        ws_->handshake(host_, "/");

        connected_ = true;
        logger_->info("Successfully connected to Binance WebSocket");

        // Start reading
        do_read();
        
        return true;
    } catch (const std::exception& e) {
        logger_->error("Failed to connect: {}", e.what());
        return false;
    }
}

void BinanceAdapter::disconnect() {
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

        logger_->info("Disconnected from Binance WebSocket");
    } catch (const std::exception& e) {
        logger_->error("Error during disconnect: {}", e.what());
    }
}

bool BinanceAdapter::isConnected() const {
    return connected_;
}

bool BinanceAdapter::subscribe(const std::vector<std::string>& symbols) {
    if (!connected_) {
        logger_->error("Cannot subscribe: WebSocket is not connected");
        return false;
    }

    try {
        // Create subscription message with correct format for Binance
        json subscribe_msg = {
            {"method", "SUBSCRIBE"},
            {"params", json::array()},
            {"id", 1}
        };

        // Add streams for each symbol
        for (const auto& symbol : symbols) {
            std::string stream_name = symbol + "@depth@100ms";
            subscribe_msg["params"].push_back(stream_name);
        }

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

bool BinanceAdapter::unsubscribe(const std::vector<std::string>& symbols) {
    if (!connected_) {
        logger_->error("Not connected");
        return false;
    }

    try {
        json unsubscribe_msg = {
            {"method", "UNSUBSCRIBE"},
            {"params", json::array()},
            {"id", 1}
        };

        // Add streams for each symbol
        for (const auto& symbol : symbols) {
            std::string stream_name = symbol + "@depth@100ms";
            unsubscribe_msg["params"].push_back(stream_name);
        }

        do_write(unsubscribe_msg.dump());
        logger_->info("Unsubscribed from symbols: {}", json(symbols).dump());
        return true;
    } catch (const std::exception& e) {
        logger_->error("Failed to unsubscribe: {}", e.what());
        return false;
    }
}

void BinanceAdapter::do_read() {
    ws_->async_read(
        buffer_,
        [this](beast::error_code ec, std::size_t bytes_transferred) {
            on_message(ec, bytes_transferred);
        });
}

void BinanceAdapter::do_write(const std::string& message) {
    ws_->async_write(
        net::buffer(message),
        [this](beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
            if (ec) {
                logger_->error("Write error: {}", ec.message());
            }
        });
}

void BinanceAdapter::on_message(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
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
        if (data.contains("result") || data.contains("id")) {
            // This is a response to our subscription/unsubscription
            logger_->info("Received response: {}", message);
            return;
        }
        
        // Handle order book updates
        if (data.contains("e") && data["e"] == "depthUpdate") {
            handle_l2update_message(data);
        } else if (data.contains("lastUpdateId")) {
            // This is a snapshot
            handle_snapshot_message(data);
        }
    } catch (const std::exception& e) {
        logger_->error("Error processing message: {}", e.what());
    }

    // Continue reading
    do_read();
}

void BinanceAdapter::initialize_logger() {
    logger_ = spdlog::stdout_color_mt("binance_adapter");
    logger_->set_level(spdlog::level::debug);
}

void BinanceAdapter::handle_snapshot_message(const json& data) {
    try {
        OrderBookSnapshot snapshot;
        snapshot.symbol = data["symbol"].get<std::string>();
        snapshot.timestamp = data["lastUpdateId"].get<uint64_t>();
        
        // Process bids
        for (const auto& bid : data["bids"]) {
            double price = std::stod(bid[0].get<std::string>());
            double size = std::stod(bid[1].get<std::string>());
            snapshot.bids.push_back({price, size});
        }
        
        // Process asks
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

void BinanceAdapter::handle_l2update_message(const json& data) {
    try {
        OrderBookDelta delta;
        delta.symbol = data["s"].get<std::string>();
        delta.timestamp = data["u"].get<uint64_t>();
        
        // Process bid updates
        for (const auto& bid : data["b"]) {
            double price = std::stod(bid[0].get<std::string>());
            double size = std::stod(bid[1].get<std::string>());
            delta.bidUpdates.push_back({price, size});
        }
        
        // Process ask updates
        for (const auto& ask : data["a"]) {
            double price = std::stod(ask[0].get<std::string>());
            double size = std::stod(ask[1].get<std::string>());
            delta.askUpdates.push_back({price, size});
        }
        
        if (order_book_delta_callback_) {
            order_book_delta_callback_(delta);
        }
    } catch (const std::exception& e) {
        logger_->error("Error handling l2update message: {}", e.what());
    }
}

// Implement other required interface methods with [[maybe_unused]] attributes
std::string BinanceAdapter::getName() const { return "Binance"; }
bool BinanceAdapter::supportsMargin() const { return true; }
double BinanceAdapter::getFeeRate([[maybe_unused]] const std::string& symbol) const { return 0.001; } // 0.1% maker/taker fee
double BinanceAdapter::getBalance([[maybe_unused]] const std::string& asset) const { return 0.0; }
std::vector<std::pair<std::string, double>> BinanceAdapter::getAllBalances() const { return {}; }
std::string BinanceAdapter::submitOrder([[maybe_unused]] const OrderRequest& request) { return ""; }
bool BinanceAdapter::cancelOrder([[maybe_unused]] const std::string& orderId) { return false; }
bool BinanceAdapter::modifyOrder([[maybe_unused]] const std::string& orderId, 
                               [[maybe_unused]] double newPrice, 
                               [[maybe_unused]] double newSize) { return false; }
bool BinanceAdapter::requestOrderBookSnapshot([[maybe_unused]] const std::string& symbol) { return false; }

void BinanceAdapter::registerOrderBookCallback(OrderBookSnapshotHandler callback) {
    order_book_callback_ = std::move(callback);
}

void BinanceAdapter::registerOrderBookDeltaCallback(OrderBookDeltaHandler callback) {
    order_book_delta_callback_ = std::move(callback);
}

void BinanceAdapter::registerExecutionCallback(ExecutionHandler callback) {
    execution_callback_ = std::move(callback);
}

} // namespace crypto_hft 