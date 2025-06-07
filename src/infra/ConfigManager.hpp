#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <type_traits>

namespace crypto_hft {

class ConfigManager {
public:
    static ConfigManager& getInstance();

    // Load configuration from file
    bool loadConfig(const std::filesystem::path& configPath);
    
    // Get configuration values
    std::string getString(const std::string& key) const;
    int getInt(const std::string& key) const;
    double getDouble(const std::string& key) const;
    bool getBool(const std::string& key) const;
    
    // Template get method with default value
    template<typename T>
    T get(const std::string& key, const T& default_value) const {
        if (!has(key)) {
            return default_value;
        }
        return get<T>(key);
    }
    
    template<typename T>
    T get(const std::string& key) const {
        if constexpr (std::is_same_v<T, std::string>) {
            return getString(key);
        } else if constexpr (std::is_same_v<T, int>) {
            return getInt(key);
        } else if constexpr (std::is_same_v<T, double>) {
            return getDouble(key);
        } else if constexpr (std::is_same_v<T, bool>) {
            return getBool(key);
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type for ConfigManager::get");
        }
    }
    
    // Check if key exists
    bool has(const std::string& key) const;
    
    // Get nested configuration
    YAML::Node getNode(const std::string& key) const;
    
    // Register callback for configuration changes
    void registerChangeCallback(
        std::function<void(const std::string&)> callback);
    
    // Reload configuration
    bool reloadConfig();
    
    // Reload current configuration
    void reload();
    
    // Save current configuration
    bool saveConfig(const std::filesystem::path& configPath) const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    
    // Prevent copying
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Configuration data
    YAML::Node config_;
    std::filesystem::path configPath_;
    
    // Change callbacks
    std::vector<std::function<void(const std::string&)>> changeCallbacks_;
    
    // Helper functions
    void notifyChange(const std::string& key);
    std::string getValueAsString(const std::string& key) const;
    template<typename T>
    T getValue(const std::string& key) const;
};

} // namespace crypto_hft 