#include "ConfigManager.hpp"
#include <fstream>
#include <sstream>

namespace crypto_hft {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::filesystem::path& configPath) {
    try {
        config_ = YAML::LoadFile(configPath.string());
        configPath_ = configPath;
        return true;
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to load config file: " + std::string(e.what()));
    }
}

void ConfigManager::reload() {
    if (!configPath_.empty()) {
        loadConfig(configPath_);
    }
}

bool ConfigManager::has(const std::string& key) const {
    return getNode(key).IsDefined();
}

YAML::Node ConfigManager::getNode(const std::string& key) const {
    std::istringstream iss(key);
    std::string token;
    YAML::Node node = config_;

    while (std::getline(iss, token, '.')) {
        if (!node.IsMap()) {
            return YAML::Node();
        }
        node = node[token];
        if (!node.IsDefined()) {
            return YAML::Node();
        }
    }

    return node;
}

} // namespace crypto_hft 