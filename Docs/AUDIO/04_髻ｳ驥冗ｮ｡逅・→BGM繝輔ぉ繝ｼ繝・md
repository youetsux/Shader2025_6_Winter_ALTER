# 第4回 音量管理とBGMフェード

## 目標

この回では、オーディオエンジンに音量管理を追加します。

次の3種類の音量を扱います。

```text
MasterVolume : 全体の音量
SEVolume     : 効果音の音量
BGMVolume    : BGMの音量
```

さらに、BGMのフェードアウトも追加します。
シーン切り替え時に、音を急に止めず、自然に小さくできます。

---

## 今回変更するファイル

```text
Engine/Audio.h
Engine/Audio.cpp
TestScene.cpp
```

---

## 1. Engine/Audio.h に音量とフェードの関数を追加する

`Engine/Audio.h` を次のように変更します。

```cpp
#pragma once
#include <string>
#include <filesystem>

using file_path = std::filesystem::path;

namespace Audio
{
    bool Initialize();
    void Update();
    void Release();

    bool LoadSE(const std::string& name, file_path& filepath);
    void PlaySE(const std::string& name);

    bool LoadBGM(const std::string& name, file_path& filepath);
    void PlayBGM(const std::string& name, bool loop = true);
    void StopBGM();

    void SetMasterVolume(float volume);
    void SetSEVolume(float volume);
    void SetBGMVolume(float volume);

    void FadeOutBGM(float fadeTime);
}
```

---

## 2. Engine/Audio.cpp に音量用の変数を追加する

`Engine/Audio.cpp` の `namespace` 内に、音量用の変数を追加します。

```cpp
float masterVolume_ = 1.0f;
float seVolume_ = 1.0f;
float bgmVolume_ = 1.0f;
```

さらに、フェード用の変数も追加します。

```cpp
bool isBGMFadeOut_ = false;
float bgmFadeTimer_ = 0.0f;
float bgmFadeTime_ = 0.0f;
float bgmFadeStartVolume_ = 1.0f;
```

`namespace` 内は、最終的に次のような形になります。

```cpp
namespace
{
    std::unique_ptr<DirectX::AudioEngine> audioEngine_;

    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> seSounds_;
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> bgmSounds_;

    std::unique_ptr<DirectX::SoundEffectInstance> currentBGM_;
    std::string currentBGMName_;

    float masterVolume_ = 1.0f;
    float seVolume_ = 1.0f;
    float bgmVolume_ = 1.0f;

    bool isBGMFadeOut_ = false;
    float bgmFadeTimer_ = 0.0f;
    float bgmFadeTime_ = 0.0f;
    float bgmFadeStartVolume_ = 1.0f;
}
```

---

## 3. 音量を0.0f〜1.0fに収める関数を作る

`Audio.cpp` の `namespace` 内に、次の補助関数を追加します。

```cpp
float ClampVolume(float volume)
{
    if (volume < 0.0f)
    {
        return 0.0f;
    }
    if (volume > 1.0f)
    {
        return 1.0f;
    }
    return volume;
}

float GetBGMFinalVolume()
{
    return masterVolume_ * bgmVolume_;
}

float GetSEFinalVolume()
{
    return masterVolume_ * seVolume_;
}
```

音量は、基本的に `0.0f` から `1.0f` の範囲で扱います。

```text
0.0f : 無音
0.5f : 半分くらい
1.0f : 標準音量
```

---

## 4. PlaySE にSE音量を反映する

`PlaySE` を次のように変更します。

```cpp
void Audio::PlaySE(const std::string& name)
{
    auto it = seSounds_.find(name);
    if (it == seSounds_.end())
    {
        return;
    }

    it->second->Play(GetSEFinalVolume());
}
```

これで、効果音は `MasterVolume × SEVolume` の音量で鳴ります。

---

## 5. PlayBGM にBGM音量を反映する

`PlayBGM` の最後に、音量設定を追加します。

```cpp
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
    currentBGM_->SetVolume(GetBGMFinalVolume());
    currentBGM_->Play(loop);
    currentBGMName_ = name;

    isBGMFadeOut_ = false;
}
```

---

## 6. 音量設定関数を追加する

`Audio.cpp` に次の関数を追加します。

```cpp
void Audio::SetMasterVolume(float volume)
{
    masterVolume_ = ClampVolume(volume);

    if (currentBGM_ != nullptr)
    {
        currentBGM_->SetVolume(GetBGMFinalVolume());
    }
}

void Audio::SetSEVolume(float volume)
{
    seVolume_ = ClampVolume(volume);
}

void Audio::SetBGMVolume(float volume)
{
    bgmVolume_ = ClampVolume(volume);

    if (currentBGM_ != nullptr)
    {
        currentBGM_->SetVolume(GetBGMFinalVolume());
    }
}
```

再生中のBGMは、音量を変更した瞬間に反映します。
効果音は、次に鳴らす音から新しい音量になります。

---

## 7. フェードアウト開始関数を追加する

`Audio.cpp` に次の関数を追加します。

```cpp
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

    isBGMFadeOut_ = true;
    bgmFadeTimer_ = 0.0f;
    bgmFadeTime_ = fadeTime;
    bgmFadeStartVolume_ = bgmVolume_;
}
```

`fadeTime` は、何秒かけて音を小さくするかです。

```cpp
Audio::FadeOutBGM(1.0f); // 1秒で小さくする
Audio::FadeOutBGM(2.0f); // 2秒で小さくする
```

---

## 8. Updateでフェード処理を進める

`Audio::Update()` を次のように変更します。

```cpp
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
```

このエンジンはメインループを約60FPSで回しているため、ここでは `1.0f / 60.0f` を使います。
将来的に正確な `deltaTime` を作った場合は、その値を使う形に変更できます。

---

## 9. TestScene.cpp で確認する

`TestScene.cpp` の `Update()` を次のように変更します。

```cpp
void TestScene::Update()
{
    if (Input::IsKeyDown(DIK_SPACE))
    {
        Audio::PlaySE("shot");
    }

    if (Input::IsKeyDown(DIK_D))
    {
        Audio::PlaySE("damage");
    }

    if (Input::IsKeyDown(DIK_1))
    {
        Audio::SetMasterVolume(1.0f);
    }

    if (Input::IsKeyDown(DIK_2))
    {
        Audio::SetMasterVolume(0.5f);
    }

    if (Input::IsKeyDown(DIK_3))
    {
        Audio::SetMasterVolume(0.0f);
    }

    if (Input::IsKeyDown(DIK_F))
    {
        Audio::FadeOutBGM(2.0f);
    }
}
```

確認すること：

```text
1キーで音量が通常になる
2キーで音量が小さくなる
3キーで無音になる
FキーでBGMが2秒かけて小さくなる
```

---

## 今回の完成状態

この回で、Audio機能は次の形になりました。

```cpp
Audio::SetMasterVolume(0.8f);
Audio::SetSEVolume(0.7f);
Audio::SetBGMVolume(0.5f);
Audio::FadeOutBGM(2.0f);
```

次回は、ゲームオブジェクトの位置と音を結びつけるために、AudioSourceと3D Audioを追加します。
