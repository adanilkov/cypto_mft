#pragma once

#include <string>
#include <vector>
#include <functional>
#include "../utils/utils.hpp"

class IExchangeAdapter {
public:
    virtual ~IExchangeAdapter() = default;

    // Connection management
    virtual void connect() = 0;

    // Market data subscription
    virtual void subscribe(const std::vector<std::string>& symbols) = 0;

    // Order management
    virtual void sendOrder(const crypto_hft::utils::OrderRequest& request) = 0;
    virtual void cancelOrder(const crypto_hft::utils::OrderCancelRequest& request) = 0;

    // Callback registration
    virtual void onMarketData(std::function<void(const crypto_hft::utils::MarketUpdate&)> cb) = 0;
    virtual void onExecutionReport(std::function<void(const crypto_hft::utils::ExecReport&)> cb) = 0;
}; 