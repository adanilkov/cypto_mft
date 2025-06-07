#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

namespace crypto_hft {

class MetricsReporter {
public:
    static MetricsReporter& getInstance();

    // Initialize metrics reporter
    void initialize(const std::string& host = "localhost",
                   int port = 9090);
    
    // Counter metrics
    void incrementCounter(const std::string& name,
                         const std::unordered_map<std::string, std::string>& labels = {});
    void addCounter(const std::string& name,
                   double value,
                   const std::unordered_map<std::string, std::string>& labels = {});
    
    // Gauge metrics
    void setGauge(const std::string& name,
                 double value,
                 const std::unordered_map<std::string, std::string>& labels = {});
    void incrementGauge(const std::string& name,
                       double value = 1.0,
                       const std::unordered_map<std::string, std::string>& labels = {});
    void decrementGauge(const std::string& name,
                       double value = 1.0,
                       const std::unordered_map<std::string, std::string>& labels = {});
    
    // Histogram metrics
    void observeHistogram(const std::string& name,
                         double value,
                         const std::unordered_map<std::string, std::string>& labels = {});
    
    // Latency measurement
    class ScopedTimer {
    public:
        ScopedTimer(MetricsReporter& reporter,
                   const std::string& name,
                   const std::unordered_map<std::string, std::string>& labels = {});
        ~ScopedTimer();
    private:
        MetricsReporter& reporter_;
        std::string name_;
        std::unordered_map<std::string, std::string> labels_;
        std::chrono::steady_clock::time_point start_;
    };
    
    // Create a scoped timer
    ScopedTimer createTimer(const std::string& name,
                           const std::unordered_map<std::string, std::string>& labels = {});

private:
    MetricsReporter() = default;
    ~MetricsReporter() = default;
    
    // Prevent copying
    MetricsReporter(const MetricsReporter&) = delete;
    MetricsReporter& operator=(const MetricsReporter&) = delete;
    
    // Prometheus registry
    std::shared_ptr<prometheus::Registry> registry_;
    
    // Metric families
    std::unordered_map<std::string, std::shared_ptr<prometheus::Counter>> counters_;
    std::unordered_map<std::string, std::shared_ptr<prometheus::Gauge>> gauges_;
    std::unordered_map<std::string, std::shared_ptr<prometheus::Histogram>> histograms_;
    
    // Helper functions
    std::shared_ptr<prometheus::Counter> getOrCreateCounter(
        const std::string& name,
        const std::unordered_map<std::string, std::string>& labels);
    std::shared_ptr<prometheus::Gauge> getOrCreateGauge(
        const std::string& name,
        const std::unordered_map<std::string, std::string>& labels);
    std::shared_ptr<prometheus::Histogram> getOrCreateHistogram(
        const std::string& name,
        const std::unordered_map<std::string, std::string>& labels);
};

// Convenience macros
#define METRIC_COUNTER(name, ...) MetricsReporter::getInstance().incrementCounter(name, __VA_ARGS__)
#define METRIC_GAUGE(name, value, ...) MetricsReporter::getInstance().setGauge(name, value, __VA_ARGS__)
#define METRIC_HISTOGRAM(name, value, ...) MetricsReporter::getInstance().observeHistogram(name, value, __VA_ARGS__)
#define METRIC_TIMER(name, ...) auto timer = MetricsReporter::getInstance().createTimer(name, __VA_ARGS__)

} // namespace crypto_hft 