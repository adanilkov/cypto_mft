#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

namespace crypto_hft {

struct PairMetrics {
    double volume_24h;
    double price_volatility;
    double spread;
    double liquidity_score;
    double correlation_score;
};

class PairSelector {
public:
    PairSelector(const std::string& config_path);
    ~PairSelector() = default;

    // Prevent copying
    PairSelector(const PairSelector&) = delete;
    PairSelector& operator=(const PairSelector&) = delete;

    // Pair selection methods
    std::vector<std::string> select_pairs(const std::vector<std::string>& available_pairs);
    void update_metrics(const std::string& pair, const PairMetrics& metrics);
    void remove_pair(const std::string& pair);
    
    // Pair analysis
    PairMetrics get_pair_metrics(const std::string& pair) const;
    double calculate_pair_score(const std::string& pair) const;
    std::vector<std::string> get_top_pairs(size_t count) const;

    // Configuration
    void set_min_volume(double min_volume);
    void set_max_spread(double max_spread);
    void set_min_liquidity_score(double min_score);
    void set_max_correlation(double max_correlation);

private:
    std::shared_ptr<spdlog::logger> logger_;
    YAML::Node config_;
    std::unordered_map<std::string, PairMetrics> pair_metrics_;

    // Configuration parameters
    double min_volume_;
    double max_spread_;
    double min_liquidity_score_;
    double max_correlation_;

    void initialize_logger();
    void load_config(const std::string& config_path);
    void validate_config() const;
    bool is_pair_eligible(const std::string& pair) const;
};

} // namespace crypto_hft 