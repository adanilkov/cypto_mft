#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>  // for std::getenv
#include <filesystem>  // for path operations
#include <fstream>  // for file operations
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <jwt-cpp/jwt.h>

namespace crypto_hft {
namespace utils {

    bool load_env_file(const std::string& path) {
        std::ifstream env_file(path);
        if (!env_file.is_open()) {
            // Try the relative path from project root
            std::filesystem::path project_root = std::filesystem::current_path();
            std::string rel_path = (project_root / ".env").string();
            env_file.open(rel_path);
            if (!env_file.is_open()) {
                std::cerr << "Failed to open .env file at: " << path << " or " << rel_path << std::endl;
                return false;
            }
        }

        std::string line;
        while (std::getline(env_file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;

            // Find the position of the equals sign
            size_t pos = line.find('=');
            if (pos == std::string::npos) continue;

            // Split into key and value
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Remove quotes if present
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }

            // Set the environment variable
            #ifdef _WIN32
                _putenv_s(key.c_str(), value.c_str());
            #else
                setenv(key.c_str(), value.c_str(), 1);
            #endif
        }
        return true;
    }

    std::string coinbase_create_jwt() {
        // Always load the .env file first
        crypto_hft::utils::load_env_file();

        const char* key_name_env = std::getenv("COINBASE_KEY_NAME");
        const char* key_secret_env = std::getenv("COINBASE_KEY_SECRET");

        // Ensure API key environment variables are present
        if (!key_name_env || !key_secret_env) {
            throw std::runtime_error("Missing required environment variables: COINBASE_KEY_NAME or COINBASE_KEY_SECRET");
        }

        std::string key_name = key_name_env;
        std::string key_secret = key_secret_env;

        // Replace all literal \n with real newlines
        size_t pos = 0;
        while ((pos = key_secret.find("\\n", pos)) != std::string::npos) {
            key_secret.replace(pos, 2, "\n");
        }

        // Logging for debugging
        std::cout << "[coinbase_create_jwt] key_name: " << key_name << std::endl;
        if (key_secret.length() > 40) {
            std::cout << "[coinbase_create_jwt] key_secret (first 20): " << key_secret.substr(0, 20) << std::endl;
            std::cout << "[coinbase_create_jwt] key_secret (last 20): " << key_secret.substr(key_secret.length() - 20) << std::endl;
        } else {
            std::cout << "[coinbase_create_jwt] key_secret: " << key_secret << std::endl;
        }

        // Generate a random nonce
        unsigned char nonce_raw[16];
        RAND_bytes(nonce_raw, sizeof(nonce_raw));
        std::string nonce(reinterpret_cast<char*>(nonce_raw), sizeof(nonce_raw));

        // Create JWT token
        auto token = jwt::create()
            .set_subject(key_name)
            .set_issuer("cdp")
            .set_not_before(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{120})
            .set_header_claim("kid", jwt::claim(key_name))
            .set_header_claim("nonce", jwt::claim(nonce))
            .sign(jwt::algorithm::es256(key_name, key_secret));

        return token;
    }
} // namespace utils
} // namespace crypto_hft 