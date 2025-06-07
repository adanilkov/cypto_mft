#pragma once

#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <optional>
#include <cmath>

namespace crypto_hft {

class SignalGenerator {
public:
    SignalGenerator(size_t windowSize);
    ~SignalGenerator();

    // Update price series
    void updatePrice(const std::string& symbol, double price, int64_t timestamp);
    
    // Calculate z-score for a pair of symbols
    std::optional<double> calculateZScore(const std::string& symbol1,
                                        const std::string& symbol2,
                                        double beta) const;
    
    // Calculate rolling statistics
    double getRollingMean(const std::string& symbol) const;
    double getRollingStdDev(const std::string& symbol) const;
    
    // Calculate correlation between two symbols
    double getCorrelation(const std::string& symbol1,
                         const std::string& symbol2) const;
    
    // Calculate cointegration test statistic (ADF)
    double getADFStatistic(const std::string& symbol1,
                          const std::string& symbol2,
                          double beta) const;
    
    // Kalman filter update (for dynamic beta estimation)
    void updateKalmanFilter(const std::string& symbol1,
                           const std::string& symbol2,
                           double measurement);
    
    // Get current Kalman filter state
    std::pair<double, double> getKalmanState(const std::string& symbol1,
                                            const std::string& symbol2) const;

private:
    // Price series storage
    struct PriceSeries {
        std::deque<double> prices;
        std::deque<int64_t> timestamps;
        double sum;
        double sumSquared;
    };
    std::unordered_map<std::string, PriceSeries> priceSeries_;
    
    // Kalman filter state
    struct KalmanState {
        double beta;      // Current beta estimate
        double variance;  // Current variance estimate
    };
    std::unordered_map<std::string, KalmanState> kalmanStates_;
    
    // Window size for rolling calculations
    size_t windowSize_;
    
    // Helper functions
    void updateRollingStats(PriceSeries& series, double price);
    double calculateBeta(const std::string& symbol1,
                        const std::string& symbol2) const;
    double calculateResiduals(const std::string& symbol1,
                            const std::string& symbol2,
                            double beta) const;
};

} // namespace crypto_hft 