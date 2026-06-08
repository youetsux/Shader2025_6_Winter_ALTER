# 第5回 AudioSourceと3D Audio

## 目標

この回では、音に位置を持たせます。

通常の効果音は、画面全体に同じように聞こえます。
3D Audioでは、音源の位置とカメラの位置を使って、左右の聞こえ方や距離による音量変化を作ります。

この回の最終形は次の使い方です。

```cpp
Audio::SetListener(cameraPosition, cameraForward, cameraUp);
Audio::PlaySE3D("explosion", soundPosition);
```

さらに、音を鳴らす部品として `AudioSource` クラスを作ります。

---

## 今回変更するファイル

```text
Engine/Audio.h
Engine/Audio.cpp
Engine/AudioSource.h      新規作成
Engine/AudioSource.cpp    新規作成
TestScene.h
TestScene.cpp
```

使用する音声ファイルを次の場所に用意してください。

```text
Assets/Audio/SE/explosion.wav
```

---

## 1. Engine/Audio.h に3D Audio用の関数を追加する

`Engine/Audio.h` に `DirectXMath.h` を追加します。

```cpp
#include <DirectXMath.h>
```

次に、`namespace Audio` の中に、次の関数を追加します。

```cpp
void SetListener(
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT3& forward,
    const DirectX::XMFLOAT3& up
);

void PlaySE3D(
    const std::string& name,
    const DirectX::XMFLOAT3& position
);
```

`Audio.h` 全体は、だいたい次のようになります。

```cpp
#pragma once
#include <string>
#include <filesystem>
#include <DirectXMath.h>

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

    void SetListener(
        const DirectX::XMFLOAT3& position,
        const DirectX::XMFLOAT3& forward,
        const DirectX::XMFLOAT3& up
    );

    void PlaySE3D(
        const std::string& name,
        const DirectX::XMFLOAT3& position
    );
}
```

---

## 2. Engine/Audio.cpp にListenerと3D再生処理を追加する

`Engine/Audio.cpp` の include に、`vector` を追加します。

```cpp
#include <vector>
```

`namespace` 内に、3D Audio用の変数を追加します。

```cpp
DirectX::AudioListener listener_;
std::vector<std::unique_ptr<DirectX::SoundEffectInstance>> active3DSE_;
```

`SetListener` を追加します。

```cpp
void Audio::SetListener(
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT3& forward,
    const DirectX::XMFLOAT3& up
)
{
    listener_.SetPosition(position);
    listener_.SetOrientation(forward, up);
}
```

`PlaySE3D` を追加します。

```cpp
void Audio::PlaySE3D(
    const std::string& name,
    const DirectX::XMFLOAT3& position
)
{
    auto it = seSounds_.find(name);
    if (it == seSounds_.end())
    {
        return;
    }

    DirectX::AudioEmitter emitter;
    emitter.SetPosition(position);

    auto instance = it->second->CreateInstance(
        DirectX::SoundEffectInstance_Use3D
    );

    instance->Apply3D(listener_, emitter);
    instance->SetVolume(GetSEFinalVolume());
    instance->Play();

    active3DSE_.push_back(std::move(instance));
}
```

3D Audioでは、次の2つを使います。

```text
AudioListener : 聞く側
AudioEmitter  : 音を出す側
```

このエンジンでは、最初はカメラを `AudioListener` として扱います。

---

## 3. Releaseで3D効果音を解放する

`Audio::Release()` の中に、次の処理を追加します。

```cpp
active3DSE_.clear();
```

`Release()` は、だいたい次のようになります。

```cpp
void Audio::Release()
{
    active3DSE_.clear();
    currentBGM_.reset();
    bgmSounds_.clear();
    seSounds_.clear();
    audioEngine_.reset();
}
```

---

## 4. Engine/AudioSource.h を作成する

`Engine` フォルダに `AudioSource.h` を作成します。

```cpp
#pragma once
#include <string>
#include <filesystem>
#include <DirectXMath.h>

using file_path = std::filesystem::path;

class AudioSource
{
public:
    AudioSource();

    void SetClip(const std::string& name);
    void SetPosition(const DirectX::XMFLOAT3& position);
    void Set3D(bool is3D);
    void Play();

private:
    std::string clipName_;
    DirectX::XMFLOAT3 position_;
    bool is3D_;
};
```

`AudioSource` は、音を鳴らすための小さな部品です。

---

## 5. Engine/AudioSource.cpp を作成する

`Engine` フォルダに `AudioSource.cpp` を作成します。

```cpp
#include "AudioSource.h"
#include "Audio.h"

AudioSource::AudioSource()
    : position_{ 0.0f, 0.0f, 0.0f }
    , is3D_(false)
{
}

void AudioSource::SetClip(const std::string& name)
{
    clipName_ = name;
}

void AudioSource::SetPosition(const DirectX::XMFLOAT3& position)
{
    position_ = position;
}

void AudioSource::Set3D(bool is3D)
{
    is3D_ = is3D;
}

void AudioSource::Play()
{
    if (clipName_.empty())
    {
        return;
    }

    if (is3D_)
    {
        Audio::PlaySE3D(clipName_, position_);
    }
    else
    {
        Audio::PlaySE(clipName_);
    }
}
```

Visual Studioのソリューションエクスプローラーで、`AudioSource.h` と `AudioSource.cpp` をプロジェクトに追加してください。

---

## 6. TestScene.h にAudioSourceを追加する

`TestScene.h` に include を追加します。

```cpp
#include "Engine/AudioSource.h"
```

`TestScene` クラスにメンバ変数を追加します。

```cpp
private:
    AudioSource explosionSound_;
```

---

## 7. TestScene.cpp で3D Audioを確認する

`TestScene.cpp` の include に、次を追加します。

```cpp
#include "Engine/Camera.h"
#include <DirectXMath.h>
```

`Initialize()` に、3D用の効果音読み込みを追加します。

```cpp
void TestScene::Initialize()
{
    Instantiate<Stage>(this);

    file_path shotPath = "Assets/Audio/SE/shot.wav";
    file_path damagePath = "Assets/Audio/SE/damage.wav";
    file_path explosionPath = "Assets/Audio/SE/explosion.wav";
    file_path stagePath = "Assets/Audio/BGM/stage.wav";

    Audio::LoadSE("shot", shotPath);
    Audio::LoadSE("damage", damagePath);
    Audio::LoadSE("explosion", explosionPath);
    Audio::LoadBGM("stage", stagePath);

    Audio::PlayBGM("stage", true);

    explosionSound_.SetClip("explosion");
    explosionSound_.Set3D(true);
    explosionSound_.SetPosition({ 3.0f, 0.0f, 0.0f });
}
```

`Update()` の先頭に、聞く側の情報を設定します。

```cpp
void TestScene::Update()
{
    DirectX::XMFLOAT3 cameraPosition;
    DirectX::XMStoreFloat3(&cameraPosition, Camera::GetPosition());

    Audio::SetListener(
        cameraPosition,
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f }
    );

    if (Input::IsKeyDown(DIK_SPACE))
    {
        Audio::PlaySE("shot");
    }

    if (Input::IsKeyDown(DIK_D))
    {
        Audio::PlaySE("damage");
    }

    if (Input::IsKeyDown(DIK_E))
    {
        explosionSound_.Play();
    }

    if (Input::IsKeyDown(DIK_F))
    {
        Audio::FadeOutBGM(2.0f);
    }
}
```

この確認では、音源位置を次に固定しています。

```cpp
{ 3.0f, 0.0f, 0.0f }
```

カメラから見て右側に音源があるため、左右の聞こえ方が変われば成功です。

---

## 8. ビルドして実行する

確認すること：

```text
Eキーでexplosion.wavが鳴る
通常のPlaySEとは違い、左右の聞こえ方が変わる
FキーでBGMがフェードアウトする
```

ヘッドホンか左右が分かるスピーカーで確認すると分かりやすいです。

---

## 注意点

### 3D Audioは反響や壁判定までは行わない

今回の3D Audioで扱うのは、主に次の内容です。

```text
左右の聞こえ方
距離による音量変化
音源位置と聞く側の位置
```

壁で音がこもる、洞窟で反響する、といった処理は別の発展内容です。

### Listenerの向きは固定している

今回の確認では、聞く向きを次のように固定しています。

```cpp
{ 0.0f, 0.0f, 1.0f }
```

カメラが自由に回転するゲームでは、カメラの向きから forward を計算して設定する必要があります。

---

## 今回の完成状態

この回で、Audio機能は次の形まで拡張されました。

```cpp
Audio::PlaySE("shot");
Audio::PlayBGM("stage", true);
Audio::SetMasterVolume(0.8f);
Audio::FadeOutBGM(2.0f);

Audio::SetListener(cameraPosition, cameraForward, cameraUp);
Audio::PlaySE3D("explosion", soundPosition);
```

さらに、GameObject側で使いやすい部品として `AudioSource` を追加しました。

```cpp
AudioSource sound;
sound.SetClip("explosion");
sound.Set3D(true);
sound.SetPosition({ 3.0f, 0.0f, 0.0f });
sound.Play();
```

これで、DirectXTK Audioを使った基本的なオーディオエンジンは完成です。
