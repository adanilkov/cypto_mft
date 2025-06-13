#include "ConfigManager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <spdlog/spdlog.h>

namespace crypto_hft {

bool ConfigManager::loadFromFile(const std::string& config_path) {
    try {
        configPath_ = std::filesystem::path(config_path);
        config_ = YAML::LoadFile(config_path);
        return validateConfig();
    } catch (const std::exception& e) {
        spdlog::error("Failed to load config from file {}: {}", config_path, e.what());
        return false;
    }
}

bool ConfigManager::loadFromString(const std::string& config_str) {
    try {
        spdlog::debug("Loading config from string: {}", config_str);
        config_ = YAML::Load(config_str);
        spdlog::debug("Config loaded successfully. Root node type: {}", config_.Type());
        if (config_.IsMap()) {
            spdlog::debug("Config is a map with {} keys", config_.size());
            for (const auto& pair : config_) {
                spdlog::debug("Key: {}, Type: {}", pair.first.as<std::string>(), pair.second.Type());
            }
        }
        return validateConfig();
    } catch (const std::exception& e) {
        spdlog::error("Failed to load config from string: {}", e.what());
        return false;
    }
}

void ConfigManager::reload() {
    if (!configPath_.empty()) {
        loadFromFile(configPath_.string());
    }
}

bool ConfigManager::has(const std::string& key) const {
    YAML::Node node = getNode(key);
    return node.IsDefined() && !node.IsNull();
}

YAML::Node ConfigManager::getNode(const std::string& key) const {
    std::istringstream iss(key);
    std::string token;
    YAML::Node node = YAML::Clone(config_);  // Make a deep copy of the root node
    spdlog::debug("Getting node for key: {}", key);
    spdlog::debug("Initial node type: {}", node.Type());

    while (std::getline(iss, token, '.'), !token.empty()) {
        spdlog::debug("Processing token: {}", token);
        if (!node.IsMap()) {
            spdlog::debug("Node is not a map, returning empty node");
            return YAML::Node();
        }

        if (!node[token].IsDefined()) {
            spdlog::debug("Token '{}' not found in node", token);
            return YAML::Node();
        }

        node = YAML::Clone(node[token]);  // Make a deep copy of the next node
        spdlog::debug("Node type after token '{}': {}", token, node.Type());
    }

    spdlog::debug("Final node type: {}", node.Type());
    return node;
}

std::string ConfigManager::getString(const std::string& key) const {
    YAML::Node node = getNode(key);
    if (!node.IsDefined() || !node.IsScalar()) {
        throw std::runtime_error("Key not found or not a string: " + key);
    }
    return node.as<std::string>();
}

int ConfigManager::getInt(const std::string& key) const {
    YAML::Node node = getNode(key);
    if (!node.IsDefined() || !node.IsScalar()) {
        throw std::runtime_error("Key not found or not an integer: " + key);
    }
    return node.as<int>();
}

double ConfigManager::getDouble(const std::string& key) const {
    YAML::Node node = getNode(key);
    if (!node.IsDefined() || !node.IsScalar()) {
        throw std::runtime_error("Key not found or not a double: " + key);
    }
    return node.as<double>();
}

bool ConfigManager::getBool(const std::string& key) const {
    YAML::Node node = getNode(key);
    if (!node.IsDefined() || !node.IsScalar()) {
        throw std::runtime_error("Key not found or not a boolean: " + key);
    }
    return node.as<bool>();
}

std::string ConfigManager::getString(const std::string& key, const std::string& default_value) const {
    try {
        return getString(key);
    } catch (const std::exception& e) {
        spdlog::error("Failed to get string for key {}: {}", key, e.what());
        return default_value;
    }
}

int ConfigManager::getInt(const std::string& key, int default_value) const {
    try {
        return getInt(key);
    } catch (const std::exception& e) {
        spdlog::error("Failed to get int for key {}: {}", key, e.what());
        return default_value;
    }
}

double ConfigManager::getDouble(const std::string& key, double default_value) const {
    try {
        return getDouble(key);
    } catch (const std::exception& e) {
        spdlog::error("Failed to get double for key {}: {}", key, e.what());
        return default_value;
    }
}

bool ConfigManager::getBool(const std::string& key, bool default_value) const {
    try {
        return getBool(key);
    } catch (const std::exception& e) {
        spdlog::error("Failed to get bool for key {}: {}", key, e.what());
        return default_value;
    }
}

std::vector<std::string> ConfigManager::getStringVector(const std::string& key) const {
    try {
        YAML::Node node = getNode(key);
        if (!node.IsDefined() || !node.IsSequence()) {
            throw std::runtime_error("Key not found or not a sequence: " + key);
        }
        return node.as<std::vector<std::string>>();
    } catch (const std::exception& e) {
        spdlog::error("Failed to get string vector for key {}: {}", key, e.what());
        return {};
    }
}

std::vector<std::string> ConfigManager::getStringVector(const std::string& key, const std::vector<std::string>& defaultValue) const {
    try {
        return getStringVector(key);
    } catch (const std::exception& e) {
        spdlog::error("Failed to get string vector for key {}: {}", key, e.what());
        return defaultValue;
    }
}

bool ConfigManager::validateConfig() const {
    // Add any necessary validation here
    return true;
}

} // namespace crypto_hft 