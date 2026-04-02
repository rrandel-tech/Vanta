#include "vapch.hpp"
#include "FileSystem.hpp"

namespace Vanta {

    bool FileSystem::CreateDirectory(const std::filesystem::path& directory)
    {
        return std::filesystem::create_directories(directory);
    }

    bool FileSystem::CreateDirectory(const std::string& directory)
    {
        return CreateDirectory(std::filesystem::path(directory));
    }

    bool FileSystem::Exists(const std::filesystem::path& filepath)
    {
        return std::filesystem::exists(filepath);
    }

    bool FileSystem::Exists(const std::string& filepath)
    {
        return std::filesystem::exists(std::filesystem::path(filepath));
    }

    bool FileSystem::ShowFileInExplorer(const std::filesystem::path& path)
    {
        auto absolutePath = std::filesystem::canonical(path);
        if (!Exists(absolutePath))
            return false;

        std::string cmd = std::format("explorer.exe /select,\"{0}\"", absolutePath.string());
        system(cmd.c_str());
        return true;
    }

    bool FileSystem::OpenDirectoryInExplorer(const std::filesystem::path& path)
    {
        auto absolutePath = std::filesystem::canonical(path);
        if (!Exists(absolutePath))
            return false;

        ShellExecuteW(NULL, L"explore", absolutePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return true;
    }

    bool FileSystem::OpenExternally(const std::filesystem::path& path)
    {
        auto absolutePath = std::filesystem::canonical(path);
        if (!Exists(absolutePath))
            return false;

        ShellExecuteW(NULL, L"open", absolutePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return true;
    }

}