#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <spdlog/spdlog.h>

namespace crypto_hft {

namespace ExchangeUtils {

// Time utilities
std::string get_timestamp();
std::string get_iso_timestamp();
int64_t get_timestamp_ms();

// Signature generation
std::string generate_hmac_sha256(const std::string& key, const std::string& data);
std::string generate_hmac_sha512(const std::string& key, const std::string& data);
std::string generate_signature(const std::string& secret, const std::string& data, const std::string& algorithm = "sha256");

// Base64 encoding/decoding
std::string base64_encode(const std::string& input);
std::string base64_decode(const std::string& input);

// URL encoding
std::string url_encode(const std::string& input);
std::string url_decode(const std::string& input);

// Symbol/currency pair utilities
std::string normalize_symbol(const std::string& symbol);
std::string denormalize_symbol(const std::string& symbol);
std::pair<std::string, std::string> split_symbol(const std::string& symbol);
std::string join_symbol(const std::string& base, const std::string& quote);

// Price and quantity formatting
std::string format_price(double price, int precision);
std::string format_quantity(double quantity, int precision);
double round_price(double price, int precision);
double round_quantity(double quantity, int precision);

// Error handling
class ExchangeError : public std::runtime_error {
public:
    explicit ExchangeError(const std::string& message) : std::runtime_error(message) {}
};

// Rate limiting
class RateLimiter {
public:
    RateLimiter(int max_requests, std::chrono::milliseconds time_window);
    bool try_acquire();

private:
    int max_requests_;
    std::chrono::milliseconds time_window_;
    std::vector<std::chrono::steady_clock::time_point> request_times_;
};

} // namespace ExchangeUtils

} // namespace crypto_hft 