#include "Audio.h"

#include <Audio.h>
#include <memory>
#include <unordered_map>



//ToDo01	Engine / Audio.h	PlayBGM に bool loop = true を追加
//ToDo02	Engine / Audio.cpp	PlayBGM の引数を bool loop に変更 → Play(loop) に渡す
//ToDo03	TestScene.cpp	#include "Engine/Audio.h" を追加
//ToDo04	TestScene.cpp Initialize()	SE / BGMをロードしてBGMを再生
//ToDo05	TestScene.cpp Update()	キー入力でSE再生・BGM停止


namespace
{
    std::unique_ptr<DirectX::AudioEngine> audioEngine_;
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> seSounds_;
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> bgmSounds_;

	std::unique_ptr<DirectX::SoundEffectInstance> currentBGM_;
	std::string currentBGMName_;
	//生ポインタで管理する場合
    //DirectX::AudioEngine* audioEngine_ = nullptr;
    //std::unordered_map<std::string, DirectX::SoundEffect*> sounds_;
}


bool Audio::Initialize()
{
    audioEngine_ = std::make_unique<DirectX::AudioEngine>();
	//audioEngine_ = new DirectX::AudioEngine();
    return true;
}

void Audio::Update()
{
    if (audioEngine_ != nullptr)
    {
        audioEngine_->Update();
    }
}

void Audio::Release()
{
    //sounds_.clear();
	currentBGM_.reset();
	bgmSounds_.clear();
	seSounds_.clear();
    audioEngine_.reset();
	//生ポインタで管理するときは、順番に気を付けてdeleteする必要がある
    //for (auto& sound : sounds_)
    //{
    //    delete sound.second;
    //    sound.second = nullptr;
    //}
    //sounds_.clear();
    //delete audioEngine_;
    //audioEngine_ = nullptr;
}

bool Audio::LoadSE(const std::string& name, file_path& filepath)
{
    if (audioEngine_ == nullptr)
    {
        return false;
    }

    seSounds_[name] = std::make_unique<DirectX::SoundEffect>(
        audioEngine_.get(),
        filepath.c_str()
    );
	//生ポインタで管理する場合
    //if (audioEngine_ == nullptr)
    //{
    //    return false;
    //}
    //DirectX::SoundEffect* sound = new DirectX::SoundEffect(
    //    audioEngine_,
    //    filepath.c_str()
    //);
    //sounds_[name] = sound;


    return true;
}

void Audio::PlaySE(const std::string& name)
{
    auto it = seSounds_.find(name);
    if (it == seSounds_.end())
    {
        return;
    }

    it->second->Play();
}

bool Audio::LoadBGM(const std::string& name, file_path& filepath)
{
	if (audioEngine_ == nullptr)
	{
		return false;
	}
	bgmSounds_[name] = std::make_unique<DirectX::SoundEffect>(
		audioEngine_.get(),
		filepath.c_str()
	);
	return true;
}

// ToDo02: 引数にloopを追加して、Play()に渡す
void Audio::PlayBGM(const std::string& name, bool loop)
{
	auto it = bgmSounds_.find(name);
	if (it == bgmSounds_.end())
	{
		return;
	}
	if (currentBGM_ != nullptr)
    {
        currentBGM_->Stop(true);
		currentBGM_.reset();
	}

	currentBGM_ = it->second->CreateInstance();
	currentBGM_->Play(loop);	// ToDo02: trueではなくloop変数を渡す
	currentBGMName_ = name;

}

void Audio::StopBGM()
{
    if (currentBGM_ != nullptr)
    {
        currentBGM_->Stop(true);
        currentBGM_.reset();
        currentBGMName_.clear();
    }

}
