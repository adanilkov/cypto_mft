#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>

namespace crypto_hft {

struct Position {
    std::string symbol;
    double size;  // positive for long, negative for short
    double averageEntryPrice;
    double unrealizedPnL;
    double realizedPnL;
    double marginUsed;
};

struct RiskLimits {
    double maxPositionSize;  // Maximum position size per symbol
    double maxLeverage;      // Maximum leverage
    double maxDrawdown;      // Maximum drawdown as percentage of capital
    double maxDailyLoss;     // Maximum daily loss
    double maxExposure;      // Maximum total exposure across all positions
    double maxConcentration; // Maximum concentration in a single symbol
};

class RiskEngine {
public:
    RiskEngine();
    ~RiskEngine();

    // Initialize with risk limits
    void initialize(const RiskLimits& limits);
    
    // Check if an order is allowed
    bool checkOrder(const std::string& symbol, 
                   double size,
                   double price,
                   bool isLong);
    
    // Update position after fill
    void onFill(const std::string& symbol,
               double size,
               double price,
               bool isLong);
    
    // Get current position
    Position getPosition(const std::string& symbol) const;
    
    // Get all positions
    std::vector<Position> getAllPositions() const;
    
    // Get current risk metrics
    double getTotalExposure() const;
    double getTotalPnL() const;
    double getDrawdown() const;
    
    // Check if risk limits are breached
    bool isRiskLimitBreached() const;
    
    // Reset risk metrics (e.g., at start of day)
    void reset();

private:
    // Risk limits
    RiskLimits limits_;
    
    // Position tracking
    std::unordered_map<std::string, Position> positions_;
    
    // Risk metrics
    double initialCapital_;
    double currentCapital_;
    double peakCapital_;
    double dailyPnL_;
    
    // Synchronization
    mutable std::mutex mutex_;
    
    // Helper functions
    void updatePosition(const std::string& symbol,
                       double size,
                       double price,
                       bool isLong);
    void updateRiskMetrics();
    bool checkPositionLimits(const std::string& symbol,
                            double newSize) const;
    bool checkExposureLimits(double newExposure) const;
    bool checkDrawdownLimits() const;
};

} // namespace crypto_hft 