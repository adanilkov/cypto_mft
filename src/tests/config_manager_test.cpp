#include <gtest/gtest.h>
#include "../infra/ConfigManager.hpp"
#include <filesystem>
#include <fstream>

namespace crypto_hft {
namespace test {

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use the config file from the build directory
        config_manager_.load("test_config.yaml");
    }

    ConfigManager config_manager_;
};

TEST_F(ConfigManagerTest, GetString) {
    EXPECT_EQ(config_manager_.get<std::string>("exchange.binance.api_key"), "test_api_key_123");
    EXPECT_EQ(config_manager_.get<std::string>("trading.strategy"), "momentum");
}

TEST_F(ConfigManagerTest, GetInt) {
    EXPECT_EQ(config_manager_.get<int>("exchange.binance.max_orders"), 100);
    EXPECT_EQ(config_manager_.get<int>("risk.max_leverage"), 20);
}

TEST_F(ConfigManagerTest, GetBool) {
    EXPECT_TRUE(config_manager_.get<bool>("exchange.binance.testnet"));
    EXPECT_TRUE(config_manager_.get<bool>("trading.enabled"));
}

TEST_F(ConfigManagerTest, GetDouble) {
    EXPECT_DOUBLE_EQ(config_manager_.get<double>("risk.stop_loss_pct"), 0.02);
    EXPECT_DOUBLE_EQ(config_manager_.get<double>("risk.take_profit_pct"), 0.05);
}

TEST_F(ConfigManagerTest, GetWithDefault) {
    EXPECT_EQ(config_manager_.get<std::string>("nonexistent.key", "default"), "default");
    EXPECT_EQ(config_manager_.get<int>("nonexistent.key", 42), 42);
    EXPECT_FALSE(config_manager_.get<bool>("nonexistent.key", false));
}

TEST_F(ConfigManagerTest, MissingKeyThrows) {
    EXPECT_THROW(config_manager_.get<std::string>("nonexistent.key"), std::runtime_error);
    EXPECT_THROW(config_manager_.get<int>("nonexistent.key"), std::runtime_error);
}

TEST_F(ConfigManagerTest, HasKey) {
    EXPECT_TRUE(config_manager_.has("exchange.binance.api_key"));
    EXPECT_FALSE(config_manager_.has("nonexistent.key"));
}

TEST_F(ConfigManagerTest, Reload) {
    // Modify the config file in the build directory
    std::ofstream config_file("test_config.yaml", std::ios::app);
    config_file << "\nnew_key: \"new_value\"\n";
    config_file.close();

    // Reload and check new value
    config_manager_.reload();
    EXPECT_EQ(config_manager_.get<std::string>("new_key"), "new_value");
}

} // namespace test
} // namespace crypto_hft 