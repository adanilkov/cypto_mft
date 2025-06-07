#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

namespace crypto_hft {

class ModelTrainer {
public:
    ModelTrainer(const std::string& config_path);
    ~ModelTrainer() = default;

    // Prevent copying
    ModelTrainer(const ModelTrainer&) = delete;
    ModelTrainer& operator=(const ModelTrainer&) = delete;

    // Training methods
    void train(const std::string& model_name, const std::vector<double>& features, const std::vector<double>& labels);
    void validate(const std::string& model_name, const std::vector<double>& features, const std::vector<double>& labels);
    void save_model(const std::string& model_name, const std::string& path);
    void load_model(const std::string& model_name, const std::string& path);

    // Model evaluation
    double evaluate(const std::string& model_name, const std::vector<double>& features);
    std::vector<double> predict(const std::string& model_name, const std::vector<double>& features);

    // Model management
    void add_model(const std::string& model_name, const std::string& model_type);
    void remove_model(const std::string& model_name);
    bool has_model(const std::string& model_name) const;

private:
    std::shared_ptr<spdlog::logger> logger_;
    YAML::Node config_;
    std::unordered_map<std::string, std::unique_ptr<class Model>> models_;

    void initialize_logger();
    void load_config(const std::string& config_path);
    void validate_config() const;
};

} // namespace crypto_hft 