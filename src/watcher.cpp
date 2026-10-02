#include "watcher.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

#include <windows.h>

namespace
{
constexpr DWORD notification_buffer_size = 64 * 1024;
}

WechatFileWatcher::WechatFileWatcher(
	const std::filesystem::path &directory_path,
	FileQueue &file_queue)
	: directory_path_(directory_path),
	  file_queue_(file_queue)
{
	directory_handle_ = CreateFileW(
		directory_path_.c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr);

	if (directory_handle_ == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("failed to open directory");
	}
}

WechatFileWatcher::~WechatFileWatcher()
{
	if (directory_handle_ != INVALID_HANDLE_VALUE)
	{
		CloseHandle(static_cast<HANDLE>(directory_handle_));
	}
}

void WechatFileWatcher::watch()
{
	std::array<BYTE, notification_buffer_size> notification_storage;

	for (;;)
	{
		DWORD bytes_returned = 0;
		BOOL success = ReadDirectoryChangesW(
			static_cast<HANDLE>(directory_handle_),
			notification_storage.data(),
			static_cast<DWORD>(notification_storage.size()),
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME,
			&bytes_returned,
			nullptr,
			nullptr);

		if (!success)
		{
			throw std::runtime_error("failed to read directory changes");
		}

		if (bytes_returned == 0)
		{
			continue;
		}

		BYTE *current = notification_storage.data();

		for (;;)
		{
			const auto *notification =
				reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(current);

			if (notification->Action == FILE_ACTION_ADDED)
			{
				std::wstring relative_name(
					notification->FileName,
					notification->FileNameLength / sizeof(wchar_t));
				std::filesystem::path file_path =
					directory_path_ / relative_name;

				DWORD attributes = GetFileAttributesW(file_path.c_str());
				if (attributes != INVALID_FILE_ATTRIBUTES &&
					(attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					file_queue_.push(std::move(file_path));
				}
			}

			if (notification->NextEntryOffset == 0)
			{
				break;
			}

			current += notification->NextEntryOffset;
		}
	}
}