#include "TradingSystem.hpp"
#include "infra/ConfigManager.hpp"
#include <spdlog/spdlog.h>
#include <csignal>
#include <chrono>
#include <filesystem>

int main() {
    spdlog::info("Welcome to crypto-hft!");

    try {
        // Create and load config
        auto config = crypto_hft::ConfigManager::create();
        std::string config_path = "config/config.yaml";
        if (!std::filesystem::exists(config_path)) {
            spdlog::error("Config file not found at: {}", config_path);
            return 1;
        }
        if (!config->loadFromFile(config_path)) {
            spdlog::error("Failed to load config from: {}", config_path);
            return 1;
        }

        static crypto_hft::TradingSystem* trading_system_ptr = nullptr; // using static ptr for lambda functions below
        auto trading_system = std::make_unique<crypto_hft::TradingSystem>(config);
        trading_system_ptr = trading_system.get();

        if (!trading_system->initialize()) {
            spdlog::error("Failed to initialize trading system");
            return 1;
        }

        if (!trading_system->start()) {
            spdlog::error("Failed to start trading system");
            return 1;
        }

        std::signal(SIGINT, [](int) {
            if (trading_system_ptr) {
                spdlog::info("[SIGINT] Shutting down...");
                trading_system_ptr->stop();
            }
        });

        std::signal(SIGTERM, [](int) {
            if (trading_system_ptr) {
                spdlog::info("[SIGTERM] Shutting down...");
                trading_system_ptr->stop();
            }
        });

        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        spdlog::error("Fatal Error: {}", e.what());
        return 1;
    }

    return 0;
} 