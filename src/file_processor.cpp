#include "file_processor.hpp"

#include <fstream>
#include <stdexcept>
#include <thread>
#include <utility>

FileProcessor::FileProcessor(
    std::filesystem::path source_root,
    std::filesystem::path backup_root,
        std::chrono::seconds retention_time,
        std::chrono::milliseconds stability_interval)
    : source_root_(std::move(source_root)),
      backup_root_(std::move(backup_root)),
            retention_time_(retention_time),
            stability_interval_(stability_interval)
{
}

bool FileProcessor::EarlierExpiration::operator()(
    const BackupRecord &left,
    const BackupRecord &right) const
{
    return left.expiration > right.expiration;
}

bool FileProcessor::wait_until_stable(
    const std::filesystem::path &source_path) const
{
    std::error_code error;
    std::uintmax_t previous_size =
        std::filesystem::file_size(source_path, error);

    if (error)
    {
        return false;
    }

    for (;;)
    {
        std::this_thread::sleep_for(stability_interval_);

        std::uintmax_t current_size =
            std::filesystem::file_size(source_path, error);

        if (error)
        {
            return false;
        }

        if (current_size == previous_size)
        {
            std::ifstream file(source_path, std::ios::binary);
            return file.good();
        }

        previous_size = current_size;
    }
}

void FileProcessor::process(const std::filesystem::path &source_path)
{
    if (!wait_until_stable(source_path))
    {
        return;
    }

    std::filesystem::path relative_path =
        source_path.lexically_relative(source_root_);
    std::filesystem::path backup_path =
        backup_root_ / relative_path;

    std::error_code error;
    std::filesystem::create_directories(
        backup_path.parent_path(),
        error);

    if (error)
    {
        throw std::runtime_error("failed to create backup directory");
    }

    std::filesystem::copy_file(
        source_path,
        backup_path,
        std::filesystem::copy_options::overwrite_existing,
        error);

    if (error)
    {
        throw std::runtime_error("failed to copy file");
    }

    BackupRecord record{
        std::chrono::steady_clock::now() + retention_time_,
        source_path,
        backup_path};

    {
        std::lock_guard<std::mutex> lock(cleanup_mutex_);
        pending_deletions_.push(std::move(record));
    }

    cleanup_condition_.notify_one();
}

void FileProcessor::cleanup()
{
    for (;;)
    {
        BackupRecord record;

        {
            std::unique_lock<std::mutex> lock(cleanup_mutex_);

            while (pending_deletions_.empty())
            {
                cleanup_condition_.wait(lock);
            }

            std::chrono::steady_clock::time_point expiration =
                pending_deletions_.top().expiration;

            if (cleanup_condition_.wait_until(lock, expiration) !=
                std::cv_status::timeout)
            {
                continue;
            }

            record = pending_deletions_.top();
            pending_deletions_.pop();
        }

        std::error_code error;
        if (std::filesystem::exists(record.source_path, error))
        {
            std::filesystem::remove(record.backup_path, error);
        }
    }
}