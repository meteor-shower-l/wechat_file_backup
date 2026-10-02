#include "watcher.hpp"
#include "file_processor.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
	try
	{
		FileQueue file_queue;
		std::filesystem::path source_root = L"C:\\xwechat_files";
		std::filesystem::path backup_root = L"D:\\wechat_backup";

		WechatFileWatcher watcher(source_root, file_queue);
		FileProcessor processor(
			source_root,
			backup_root,
			std::chrono::minutes(2));

		std::thread watcher_thread(&WechatFileWatcher::watch, &watcher);
		std::thread cleanup_thread(&FileProcessor::cleanup, &processor);

		for (;;)
		{
			std::filesystem::path file_path = file_queue.pop();
			processor.process(file_path);
		}

		watcher_thread.join();
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}