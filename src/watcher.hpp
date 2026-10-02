#ifndef WECHAT_FILE_BACKUP_WATCHER_HPP
#define WECHAT_FILE_BACKUP_WATCHER_HPP

#include <filesystem>

#include "file_queue.hpp"

struct WechatFileWatcher
{
    WechatFileWatcher(
        const std::filesystem::path &directory_path,
        FileQueue &file_queue);
    ~WechatFileWatcher();

    void watch();

private:
    std::filesystem::path directory_path_;
    FileQueue &file_queue_;
    void *directory_handle_ = nullptr;
};

#endif