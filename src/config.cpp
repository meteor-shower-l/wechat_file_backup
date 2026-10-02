#include "config.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

AppConfig load_config(const std::filesystem::path &config_path)
{
    std::ifstream file(config_path);

    if (!file)
    {
        throw std::runtime_error("failed to open config file");
    }

    nlohmann::json json;
    file >> json;

    AppConfig config{
        std::filesystem::u8path(
            json.at("source_root").get<std::string>()),
        std::filesystem::u8path(
            json.at("backup_root").get<std::string>()),
        std::chrono::seconds(
            json.at("retention_seconds").get<long long>()),
        std::chrono::milliseconds(
            json.at("stability_interval_milliseconds").get<long long>())};

    if (config.retention_time <= std::chrono::seconds::zero() ||
        config.stability_interval <=
            std::chrono::milliseconds::zero())
    {
        throw std::runtime_error("config durations must be positive");
    }

    return config;
}