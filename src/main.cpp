#include "config.hpp"
#include "file_processor.hpp"
#include "watcher.hpp"

#include <iostream>
#include <thread>

int main(int argc, char *argv[])
{
	try
	{
		std::filesystem::path config_path =
			argc > 1 ? std::filesystem::path(argv[1]) :
			std::filesystem::path("config.json");
		AppConfig config = load_config(config_path);

		FileQueue file_queue;

		WechatFileWatcher watcher(config.source_root, file_queue);
		FileProcessor processor(
			config.source_root,
			config.backup_root,
			config.retention_time,
			config.stability_interval);

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