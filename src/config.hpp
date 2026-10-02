#ifndef WECHAT_FILE_BACKUP_CONFIG_HPP
#define WECHAT_FILE_BACKUP_CONFIG_HPP

#include <chrono>
#include <filesystem>

struct AppConfig
{
    std::filesystem::path source_root;
    std::filesystem::path backup_root;
    std::chrono::seconds retention_time;
    std::chrono::milliseconds stability_interval;
};

AppConfig load_config(const std::filesystem::path &config_path);

#endif