#pragma once

#include "Core/Buffer.hpp"

#include <functional>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef MoveFile
#undef MoveFile
#endif

#include <filesystem>

#include <string>

namespace Vanta {

    enum class FileSystemAction
    {
        Added, Rename, Modified, Delete
    };

    struct FileSystemChangedEvent
    {
        FileSystemAction Action;
        std::string FilePath;
        std::string OldName;
        std::string NewName;
        bool IsDirectory;
        bool WasTracking = false;
    };

    class FileSystem
    {
    public:
        static bool CreateDirectory(const std::filesystem::path& directory);
        static bool CreateDirectory(const std::string& directory);
        static bool Exists(const std::filesystem::path& filepath);
        static bool Exists(const std::string& filepath);
        static std::string Rename(const std::string& filepath, const std::string& newName);
        static bool DeleteFile(const std::string& filepath);
        static bool MoveFile(const std::string& filepath, const std::string& dest);
        static bool IsDirectory(const std::string& filepath);

    public:
        using FileSystemChangedCallbackFn = std::function<void(FileSystemChangedEvent)>;

        static void SetChangeCallback(const FileSystemChangedCallbackFn& callback);
        static void StartWatching();
        static void StopWatching();

        static void SkipNextFileSystemChange();

    private:
        static unsigned long Watch(void* param);

    private:
        static FileSystemChangedCallbackFn s_Callback;
    };
}