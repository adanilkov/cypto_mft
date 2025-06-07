#include "ConfigManager.hpp"
#include <fstream>
#include <sstream>

namespace crypto_hft {

void ConfigManager::load(const std::string& filename) {
    config_file_ = filename;
    reload();
}

void ConfigManager::reload() {
    try {
        config_ = YAML::LoadFile(config_file_);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to load config file: " + std::string(e.what()));
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