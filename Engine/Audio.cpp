#include "Audio.h"

#include "Audio.h"
#include <Audio.h>
#include <memory>
#include <unordered_map>

namespace
{
    std::unique_ptr<DirectX::AudioEngine> audioEngine_;
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> sounds_;
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
    sounds_.clear();
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

bool Audio::Load(const std::string& name, file_path& filepath)
{
    if (audioEngine_ == nullptr)
    {
        return false;
    }

    sounds_[name] = std::make_unique<DirectX::SoundEffect>(
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

void Audio::Play(const std::string& name)
{
    auto it = sounds_.find(name);
    if (it == sounds_.end())
    {
        return;
    }

    it->second->Play();
}
