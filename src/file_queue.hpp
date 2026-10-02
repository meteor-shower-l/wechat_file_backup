#ifndef FILE_QUEUE_HPP
#define FILE_QUEUE_HPP

#include <array>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <utility>

class FileQueue
{
public:
	static constexpr std::size_t capacity = 64;

	void push(std::filesystem::path file_path)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		not_full_.wait(lock, [this] { return count_ < capacity; });

		paths_[tail_] = std::move(file_path);
		tail_ = (tail_ + 1) % capacity;
		++count_;

		lock.unlock();
		not_empty_.notify_one();
	}

	std::filesystem::path pop()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		not_empty_.wait(lock, [this] { return count_ > 0; });

		std::filesystem::path file_path = std::move(paths_[head_]);
		head_ = (head_ + 1) % capacity;
		--count_;

		lock.unlock();
		not_full_.notify_one();
		return file_path;
	}

private:
	std::array<std::filesystem::path, capacity> paths_;
	std::size_t head_ = 0;
	std::size_t tail_ = 0;
	std::size_t count_ = 0;
	std::mutex mutex_;
	std::condition_variable not_empty_;
	std::condition_variable not_full_;
};

#endif