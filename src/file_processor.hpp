#ifndef WECHAT_FILE_BACKUP_FILE_PROCESSOR_HPP
#define WECHAT_FILE_BACKUP_FILE_PROCESSOR_HPP

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <vector>

class FileProcessor
{
public:
    FileProcessor(
        std::filesystem::path source_root,
        std::filesystem::path backup_root,
        std::chrono::milliseconds retention_time);

    void process(const std::filesystem::path &source_path);
    void cleanup();

private:
    struct BackupRecord
    {
        std::chrono::steady_clock::time_point expiration;
        std::filesystem::path source_path;
        std::filesystem::path backup_path;
    };

    struct EarlierExpiration
    {
        bool operator()(
            const BackupRecord &left,
            const BackupRecord &right) const;
    };

    bool wait_until_stable(
        const std::filesystem::path &source_path) const;

    std::filesystem::path source_root_;
    std::filesystem::path backup_root_;
    std::chrono::milliseconds retention_time_;
    std::priority_queue<
        BackupRecord,
        std::vector<BackupRecord>,
        EarlierExpiration> pending_deletions_;
    std::mutex cleanup_mutex_;
    std::condition_variable cleanup_condition_;
};

#endif