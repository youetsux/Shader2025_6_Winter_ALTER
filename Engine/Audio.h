#pragma once
#include <string>
#include <filesystem>

using file_path = std::filesystem::path;

namespace Audio
{
    bool Initialize();
    void Update();
    void Release();

    bool Load(const std::string& name, file_path &filepath);
    void Play(const std::string& name);
};

