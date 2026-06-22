#pragma once
#include <string>
#include <filesystem>

using file_path = std::filesystem::path;

namespace Audio
{
    bool Initialize();
    void Update();
    void Release();

    bool LoadSE(const std::string& name, file_path &filepath);
    void PlaySE(const std::string& name);

	bool LoadBGM(const std::string& name, file_path& filepath);
	// ToDo01: PlayBGMにループ再生フラグを追加する（デフォルトtrue＝ループあり）
	void PlayBGM(const std::string& name, bool loop = true);
	void StopBGM();

};

