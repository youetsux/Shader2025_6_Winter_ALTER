# 第3回 効果音とBGMを分ける

## 目標

この回では、Audio機能を次の2種類に分けます。

```text
SE  : 効果音。短い音。何度も鳴る。
BGM : 背景音楽。長い音。基本的に1曲を流し続ける。
```

最終的に、次のように使える形にします。

```cpp
file_path shotPath = "Assets/Audio/SE/shot.wav";
file_path stagePath = "Assets/Audio/BGM/stage.wav";

Audio::LoadSE("shot", shotPath);
Audio::LoadBGM("stage", stagePath);

Audio::PlaySE("shot");
Audio::PlayBGM("stage", true);
```

---

## 今回変更するファイル

```text
Engine/Audio.h
Engine/Audio.cpp
TestScene.cpp
```

使用する音声ファイルを次の場所に用意してください。

```text
Assets/Audio/SE/shot.wav
Assets/Audio/SE/damage.wav
Assets/Audio/BGM/stage.wav
```

ファイル名は、資料内のコードと合わせてください。

---

## 1. Engine/Audio.h を変更する

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
}
```

`file_path` の使い方は第2回と同じです。

```cpp
file_path shotPath = "Assets/Audio/SE/shot.wav";
Audio::LoadSE("shot", shotPath);
```

`LoadSE("shot", "Assets/Audio/SE/shot.wav")` のように直接文字列を渡すのではなく、いったん `file_path` 変数に入れてから渡します。

第2回で作った `Load` と `Play` は、今回から使いません。

```text
Load    → LoadSE / LoadBGM に分ける
Play    → PlaySE / PlayBGM に分ける
```

---

## 2. Engine/Audio.cpp を変更する

`Engine/Audio.cpp` を次のように変更します。

```cpp
#include "Audio.h"
#include <Audio.h>
#include <memory>
#include <unordered_map>

namespace
{
    std::unique_ptr<DirectX::AudioEngine> audioEngine_;

    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> seSounds_;
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> bgmSounds_;

    std::unique_ptr<DirectX::SoundEffectInstance> currentBGM_;
    std::string currentBGMName_;
}

bool Audio::Initialize()
{
    audioEngine_ = std::make_unique<DirectX::AudioEngine>();
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
    currentBGM_.reset();
    bgmSounds_.clear();
    seSounds_.clear();
    audioEngine_.reset();
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
    currentBGM_->Play(loop);
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
```

---

## 3. SoundEffect と SoundEffectInstance の違い

今回から `SoundEffectInstance` が出てきます。

```text
SoundEffect          : 音声ファイルのデータ
SoundEffectInstance  : 実際に再生している音
```

効果音は、単純に `PlaySE` で鳴らします。

```cpp
Audio::PlaySE("shot");
```

BGMは、再生中の音を止めたり、ループしたりする必要があります。
そのため、`SoundEffectInstance` を使います。

```cpp
currentBGM_ = it->second->CreateInstance();
currentBGM_->Play(loop);
```

---

## 4. TestScene.cpp で使う

`TestScene.cpp` の `Initialize()` を次のように変更します。

```cpp
void TestScene::Initialize()
{
    Instantiate<Stage>(this);

    file_path shotPath = "Assets/Audio/SE/shot.wav";
    file_path damagePath = "Assets/Audio/SE/damage.wav";
    file_path stagePath = "Assets/Audio/BGM/stage.wav";

    Audio::LoadSE("shot", shotPath);
    Audio::LoadSE("damage", damagePath);
    Audio::LoadBGM("stage", stagePath);

    Audio::PlayBGM("stage", true);
}
```

`Update()` を次のように変更します。

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

    if (Input::IsKeyDown(DIK_B))
    {
        Audio::StopBGM();
    }
}
```

---

## 5. ビルドして実行する

確認すること：

```text
起動するとBGMがループ再生される
スペースキーでshot.wavが鳴る
Dキーでdamage.wavが鳴る
BキーでBGMが止まる
```

---

## よくある失敗

### BGMが鳴らない

ファイルパスを確認してください。

```cpp
"Assets/Audio/BGM/stage.wav"
```

### BGMが一瞬で止まる

`currentBGM_` をローカル変数にしていると、関数終了時に消えてしまいます。
今回のコードでは、ファイル上部の `namespace` 内に置いています。

```cpp
std::unique_ptr<DirectX::SoundEffectInstance> currentBGM_;
```

### 効果音を連打すると音が重なる

これは正常です。
効果音は、短い音を何度も鳴らす用途なので、同じ音が重なって鳴っても問題ありません。

---

## 今回の完成状態

この回で、Audio機能は次の形になりました。

```cpp
file_path shotPath = "Assets/Audio/SE/shot.wav";
file_path stagePath = "Assets/Audio/BGM/stage.wav";

Audio::LoadSE("shot", shotPath);
Audio::PlaySE("shot");

Audio::LoadBGM("stage", stagePath);
Audio::PlayBGM("stage", true);
Audio::StopBGM();
```

次回は、マスター音量、SE音量、BGM音量を分けて管理します。
