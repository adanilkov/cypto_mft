#include "ConfigManager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace crypto_hft {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::filesystem::path& configPath) {
    try {
        if (!std::filesystem::exists(configPath)) {
            throw std::runtime_error("Config file does not exist: " + configPath.string());
        }

        std::ifstream file(configPath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + configPath.string());
        }

        // Read the entire file into a string
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        if (content.empty()) {
            throw std::runtime_error("Config file is empty: " + configPath.string());
        }

        // Parse the YAML content
        config_ = YAML::Load(content);
        if (!config_.IsMap()) {
            throw std::runtime_error("Invalid config file format: root must be a map");
        }

        configPath_ = configPath;
        return true;
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to load config file: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load config file: " + std::string(e.what()));
    }
}

void ConfigManager::reload() {
    if (!configPath_.empty()) {
        loadConfig(configPath_);
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

    while (std::getline(iss, token, '.')) {
        if (!node.IsMap()) {
            return YAML::Node();
        }

        if (!node[token].IsDefined()) {
            return YAML::Node();
        }

        node = YAML::Clone(node[token]);  // Make a deep copy of the next node
    }

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

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) const {
    try {
        return getString(key);
    } catch (const std::runtime_error&) {
        return defaultValue;
    }
}

int ConfigManager::getInt(const std::string& key, int defaultValue) const {
    try {
        return getInt(key);
    } catch (const std::runtime_error&) {
        return defaultValue;
    }
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) const {
    try {
        return getBool(key);
    } catch (const std::runtime_error&) {
        return defaultValue;
    }
}

} // namespace crypto_hft 