#pragma once

#include <memory>
#include <vector>
#include <string>
#include "infra/ConfigManager.hpp"
#include "core/MarketDataEngine.hpp"
#include "core/ExecutionEngine.hpp"
#include "exchanges/IExchangeAdapter.hpp"

namespace crypto_hft {

class TradingSystem {
public:
    explicit TradingSystem(std::shared_ptr<ConfigManager> config);
    ~TradingSystem();

    bool initialize();
    bool start();
    void stop();

    std::shared_ptr<MarketDataEngine> getMarketDataEngine() const { return market_data_engine_; }

private:
    void initializeExchanges();
    void initializeMarketData();
    void initializeRisk();
    void initializeExecution();
    void initializeStrategy();

    std::shared_ptr<ConfigManager> config_manager_;
    std::vector<std::shared_ptr<IExchangeAdapter>> adapters_;
    std::shared_ptr<MarketDataEngine> market_data_engine_;
    std::shared_ptr<ExecutionEngine> execution_engine_;
    // std::shared_ptr<StrategyEngine> strategy_;
    // std::shared_ptr<RiskManager> risk_manager_;
};

} // namespace crypto_hft