#include <gtest/gtest.h>
#include "../../src/infra/ConfigManager.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace crypto_hft {
namespace test {

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test config file with known values
        config_file_ = std::filesystem::temp_directory_path() / "test_config.yaml";
        
        // Remove existing file if it exists
        if (std::filesystem::exists(config_file_)) {
            std::filesystem::remove(config_file_);
        }
        
        // Create and write config file
        std::ofstream config_file(config_file_);
        if (!config_file.is_open()) {
            throw std::runtime_error("Failed to create config file");
        }
        
        config_file << R"(
exchange:
  binance:
    api_key: "test_api_key_123"
    max_orders: 100
    testnet: true
trading:
  strategy: "momentum"
  enabled: true
risk:
  max_leverage: 20
  stop_loss_pct: 0.02
  take_profit_pct: 0.05
)";
        config_file.close();

        // Print the YAML file content for debugging
        std::ifstream in(config_file_.string());
        std::cout << "YAML file content written to: " << config_file_ << std::endl;
        std::cout << std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()) << std::endl;

        // Reset config manager and verify load
        if (!ConfigManager::getInstance().loadConfig(config_file_.string())) {
            throw std::runtime_error("Failed to load config file");
        }
    }

    void TearDown() override {
        // Clean up test config file
        if (std::filesystem::exists(config_file_)) {
            std::filesystem::remove(config_file_);
        }
    }

    ConfigManager& config_manager_ = ConfigManager::getInstance();
    std::filesystem::path config_file_;
};

TEST_F(ConfigManagerTest, GetString) {
    EXPECT_EQ(config_manager_.getString("exchange.binance.api_key"), "test_api_key_123");
    EXPECT_EQ(config_manager_.getString("trading.strategy"), "momentum");
}

TEST_F(ConfigManagerTest, GetInt) {
    EXPECT_EQ(config_manager_.getInt("exchange.binance.max_orders"), 100);
    EXPECT_EQ(config_manager_.getInt("risk.max_leverage"), 20);
}

TEST_F(ConfigManagerTest, GetBool) {
    EXPECT_TRUE(config_manager_.getBool("exchange.binance.testnet"));
    EXPECT_TRUE(config_manager_.getBool("trading.enabled"));
}

TEST_F(ConfigManagerTest, GetDouble) {
    EXPECT_DOUBLE_EQ(config_manager_.getDouble("risk.stop_loss_pct"), 0.02);
    EXPECT_DOUBLE_EQ(config_manager_.getDouble("risk.take_profit_pct"), 0.05);
}

TEST_F(ConfigManagerTest, GetWithDefault) {
    EXPECT_EQ(config_manager_.getString("nonexistent.key", "default"), "default");
    EXPECT_EQ(config_manager_.getInt("nonexistent.key", 42), 42);
    EXPECT_FALSE(config_manager_.getBool("nonexistent.key", false));
}

TEST_F(ConfigManagerTest, MissingKeyThrows) {
    EXPECT_THROW(config_manager_.getString("nonexistent.key"), std::runtime_error);
    EXPECT_THROW(config_manager_.getInt("nonexistent.key"), std::runtime_error);
}

TEST_F(ConfigManagerTest, HasKey) {
    EXPECT_TRUE(config_manager_.has("exchange.binance.api_key"));
    EXPECT_FALSE(config_manager_.has("nonexistent.key"));
}

TEST_F(ConfigManagerTest, Reload) {
    // Modify the config file
    std::ofstream config_file(config_file_, std::ios::app);
    config_file << "\nnew_key: \"new_value\"\n";
    config_file.close();

    // Reload and check new value
    config_manager_.reload();
    EXPECT_EQ(config_manager_.getString("new_key"), "new_value");
}

} // namespace test
} // namespace crypto_hft 