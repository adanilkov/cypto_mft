#include "TradingSystem.hpp"
#include "exchanges/CoinbaseAdapter.hpp"
// #include "exchanges/KrakenAdapter.hpp"
// #include "exchanges/BinanceAdapter.hpp"
#include <spdlog/spdlog.h>

namespace crypto_hft {

TradingSystem::TradingSystem(std::shared_ptr<ConfigManager> config)
    : config_manager_(std::move(config))
{
}

TradingSystem::~TradingSystem()
{
    stop();
}

bool TradingSystem::initialize()
{
    try {
        initializeExchanges();
        initializeMarketData();
        initializeRisk();
        initializeExecution();
        initializeStrategy();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize trading system: {}", e.what());
        return false;
    }
}

void TradingSystem::initializeExchanges()
{
    if (config_manager_->getBool("exchanges.coinbase.enabled", false)) {
        adapters_.push_back(std::make_shared<CoinbaseAdapter>());
    }
    // if (config_manager_->getBool("exchanges.kraken.enabled", false)) {
    //     adapters_.push_back(std::make_shared<KrakenAdapter>());
    // }
    // if (config_manager_->getBool("exchanges.binance.enabled", false)) {
    //     adapters_.push_back(std::make_shared<BinanceAdapter>());
    // }
}

void TradingSystem::initializeMarketData()
{
    market_data_engine_ = std::make_shared<MarketDataEngine>(adapters_);
    std::vector<std::string> symbols = config_manager_->getStringVector("market_data.symbols");
    market_data_engine_->initialize(symbols);

    for (auto& adapter : adapters_) {
        adapter->connect();
        adapter->subscribe(symbols);
    }
}

void TradingSystem::initializeRisk()
{
    // TODO: Implement risk management initialization
}

void TradingSystem::initializeExecution()
{
    // TODO: Implement execution engine initialization
}

void TradingSystem::initializeStrategy()
{
    // TODO: Implement strategy initialization
}

bool TradingSystem::start()
{
    try {
        // TODO: Start strategy engine
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start trading system: {}", e.what());
        return false;
    }
}

void TradingSystem::stop()
{
    // Get the symbols we're subscribed to
    std::vector<std::string> symbols = config_manager_->getStringVector("market_data.symbols");

    // Disconnect all exchange adapters
    for (auto& adapter : adapters_) {
        if (adapter) {
            adapter->unsubscribe(symbols);
            adapter->disconnect();
        }
    }

    // Reset components
    market_data_engine_.reset();
    execution_engine_.reset();
    // strategy_.reset();
    // risk_manager_.reset();
}

} // namespace crypto_hft