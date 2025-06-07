#include "CoinbaseAdapter.hpp"

namespace crypto_hft {

CoinbaseAdapter::CoinbaseAdapter(const std::string& config_path) {
    initialize_logger();
    load_config(config_path);
    validate_config();
    setup_websocket();
}

CoinbaseAdapter::~CoinbaseAdapter() {
    disconnect();
}

bool CoinbaseAdapter::connect() {
    try {
        if (connected_) {
            logger_->warn("Already connected");
            return true;
        }

        // Resolve the host
        tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(base_url_, "443");

        // Create the WebSocket stream
        ws_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(ioc_, ssl_ctx_);

        // Connect to the host
        net::connect(ws_->next_layer().next_layer(), results.begin(), results.end());
        
        // Perform SSL handshake
        ws_->next_layer().handshake(ssl::stream_base::client);

        // Perform WebSocket handshake
        ws_->handshake(base_url_, "/");

        connected_ = true;
        logger_->info("Successfully connected to {}", base_url_);
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
        if (ws_) {
            ws_->close(websocket::close_code::normal);
            ws_.reset();
        }
        connected_ = false;
        logger_->info("Disconnected from {}", base_url_);
    } catch (const std::exception& e) {
        logger_->error("Error during disconnect: {}", e.what());
    }
}

bool CoinbaseAdapter::isConnected() const {
    return connected_;
}

void CoinbaseAdapter::initialize_logger() {
    logger_ = spdlog::stdout_color_mt("coinbase_adapter");
    logger_->set_level(spdlog::level::debug);
}

void CoinbaseAdapter::load_config(const std::string& config_path) {
    try {
        config_ = YAML::LoadFile(config_path);
        base_url_ = config_["websocket_url"].as<std::string>();
        api_key_ = config_["api_key"].as<std::string>();
        api_secret_ = config_["api_secret"].as<std::string>();
        passphrase_ = config_["passphrase"].as<std::string>();
    } catch (const std::exception& e) {
        logger_->error("Failed to load config: {}", e.what());
        throw;
    }
}

void CoinbaseAdapter::validate_config() const {
    if (base_url_.empty() || api_key_.empty() || api_secret_.empty() || passphrase_.empty()) {
        throw std::runtime_error("Invalid configuration: missing required fields");
    }
}

void CoinbaseAdapter::setup_websocket() {
    // Additional WebSocket setup if needed
}

void CoinbaseAdapter::handle_websocket_message(const std::string& message) {
    // Implement message handling
    logger_->debug("Received message: {}", message);
}

// Implement other interface methods...

} // namespace crypto_hft 