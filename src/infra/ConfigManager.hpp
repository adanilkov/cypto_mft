#pragma once

#include <memory>
#include <string>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace crypto_hft {

class ConfigManager {
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    // Prevent copying
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Allow moving
    ConfigManager(ConfigManager&&) = default;
    ConfigManager& operator=(ConfigManager&&) = default;

    // Load configuration from file
    void load(const std::string& filename);

    // Reload configuration
    void reload();

    // Get configuration values with type safety
    template<typename T>
    T get(const std::string& key) const;

    // Get configuration values with default
    template<typename T>
    T get(const std::string& key, const T& defaultValue) const;

    // Check if key exists
    bool has(const std::string& key) const;

private:
    std::string config_file_;
    YAML::Node config_;

    // Helper to get node from dot-notation key
    YAML::Node getNode(const std::string& key) const;
};

// Template implementations
template<typename T>
T ConfigManager::get(const std::string& key) const {
    auto node = getNode(key);
    if (!node) {
        throw std::runtime_error("Configuration key not found: " + key);
    }
    return node.as<T>();
}

template<typename T>
T ConfigManager::get(const std::string& key, const T& defaultValue) const {
    auto node = getNode(key);
    if (!node) {
        return defaultValue;
    }
    return node.as<T>();
}

} // namespace crypto_hft 