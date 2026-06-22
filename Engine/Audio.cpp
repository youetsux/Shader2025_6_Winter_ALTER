#include "Audio.h"

#include <Audio.h>
#include <memory>
#include <unordered_map>



//ToDo01	Engine / Audio.h	PlayBGM に bool loop = true を追加
//ToDo02	Engine / Audio.cpp	PlayBGM の引数を bool loop に変更 → Play(loop) に渡す
//ToDo03	TestScene.cpp	#include "Engine/Audio.h" を追加
//ToDo04	TestScene.cpp Initialize()	SE / BGMをロードしてBGMを再生
//ToDo05	TestScene.cpp Update()	キー入力でSE再生・BGM停止


// --- 第4回: 音量管理とBGMフェード ---
// ToDo01 Audio.h              : SetMasterVolume/SetSEVolume/SetBGMVolume/FadeOutBGM の宣言追加
// ToDo02 Audio.cpp namespace  : 音量変数（masterVolume_/seVolume_/bgmVolume_）追加
// ToDo03 Audio.cpp namespace  : フェード変数（isBGMFadeOut_ 等）追加
// ToDo04 Audio.cpp namespace  : 補助関数 ClampVolume/GetBGMFinalVolume/GetSEFinalVolume 追加
// ToDo05 Audio.cpp PlaySE     : GetSEFinalVolume() を渡す
// ToDo06 Audio.cpp PlayBGM    : SetVolume と isBGMFadeOut_=false を追加
// ToDo07 Audio.cpp            : SetMasterVolume/SetSEVolume/SetBGMVolume を追加
// ToDo08 Audio.cpp            : FadeOutBGM を追加
// ToDo09 Audio.cpp Update     : フェード処理を追加
// ToDo10 TestScene.cpp Update : 音量・フェード確認用キー入力追加

namespace
{
	std::unique_ptr<DirectX::AudioEngine> audioEngine_;
	std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> seSounds_;
	std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> bgmSounds_;

	std::unique_ptr<DirectX::SoundEffectInstance> currentBGM_;
	std::string currentBGMName_;

	// ToDo02: 音量変数を追加する（0.0f〜1.0f）
	float masterVolume_ = 1.0f;
	float seVolume_     = 1.0f;
	float bgmVolume_    = 1.0f;

	// ToDo03: フェードアウト用の変数を追加する
	bool  isBGMFadeOut_       = false;
	float bgmFadeTimer_       = 0.0f;
	float bgmFadeTime_        = 0.0f;
	float bgmFadeStartVolume_ = 1.0f;

	//生ポインタで管理する場合
	//DirectX::AudioEngine* audioEngine_ = nullptr;
	//std::unordered_map<std::string, DirectX::SoundEffect*> sounds_;

	// ToDo04: 音量を0.0f〜1.0fに収める補助関数
	float ClampVolume(float volume)
	{
		if (volume < 0.0f) return 0.0f;
		if (volume > 1.0f) return 1.0f;
		return volume;
	}

	// ToDo04: 最終音量を計算する補助関数（マスター × 個別）
	float GetBGMFinalVolume() { return masterVolume_ * bgmVolume_; }
	float GetSEFinalVolume()  { return masterVolume_ * seVolume_;  }
}


bool Audio::Initialize()
{
    audioEngine_ = std::make_unique<DirectX::AudioEngine>();
	//audioEngine_ = new DirectX::AudioEngine();
    return true;
}

// ToDo09: フェードアウト処理を Update に追加する
void Audio::Update()
{
    if (audioEngine_ != nullptr)
    {
        audioEngine_->Update();
    }

    if (isBGMFadeOut_ && currentBGM_ != nullptr)
    {
        const float DELTA_TIME = 1.0f / 60.0f;

        bgmFadeTimer_ += DELTA_TIME;
        float rate = bgmFadeTimer_ / bgmFadeTime_;

        if (rate >= 1.0f)
        {
            SetBGMVolume(0.0f);
            StopBGM();
            bgmVolume_ = bgmFadeStartVolume_;
            isBGMFadeOut_ = false;
            return;
        }

        float volume = bgmFadeStartVolume_ * (1.0f - rate);
        SetBGMVolume(volume);
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

    it->second->Play(GetSEFinalVolume(), 0.0f, 0.0f); // ToDo05: SEの最終音量（Master × SE）を渡す。pitch/panは0.0f固定
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
	currentBGM_->SetVolume(GetBGMFinalVolume()); // ToDo06: BGMの最終音量を設定する
	currentBGM_->Play(loop);
	currentBGMName_ = name;
	isBGMFadeOut_ = false; // ToDo06: フェード状態をリセットする

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

// ToDo07: マスター音量を設定する（再生中のBGMに即反映）
void Audio::SetMasterVolume(float volume)
{
    masterVolume_ = ClampVolume(volume);

    if (currentBGM_ != nullptr)
    {
        currentBGM_->SetVolume(GetBGMFinalVolume());
    }
}

// ToDo07: SE音量を設定する（次の PlaySE から反映）
void Audio::SetSEVolume(float volume)
{
    seVolume_ = ClampVolume(volume);
}

// ToDo07: BGM音量を設定する（再生中のBGMに即反映）
void Audio::SetBGMVolume(float volume)
{
    bgmVolume_ = ClampVolume(volume);

    if (currentBGM_ != nullptr)
    {
        currentBGM_->SetVolume(GetBGMFinalVolume());
    }
}

// ToDo08: BGMを指定秒数でフェードアウトする
void Audio::FadeOutBGM(float fadeTime)
{
    if (currentBGM_ == nullptr)
    {
        return;
    }

    if (fadeTime <= 0.0f)
    {
        StopBGM();
        return;
    }

    isBGMFadeOut_       = true;
    bgmFadeTimer_       = 0.0f;
    bgmFadeTime_        = fadeTime;
    bgmFadeStartVolume_ = bgmVolume_;
}
