# 第2回 Audio機能をEngineフォルダに移す

## 目標

この回では、第1回で `TestScene` に直接書いた音声処理を、エンジン側の機能として分離します。

最終的に、ゲーム側から次のように使える形にします。

```cpp
file_path testPath = "Assets/Audio/test.wav";
Audio::Load("test", testPath);
Audio::Play("test");
```

DirectXTK Audioの細かい処理は、`Audio` の中に隠します。

---

## 今回変更するファイル

```text
Engine/Audio.h      新規作成
Engine/Audio.cpp    新規作成
Main.cpp            追加
TestScene.h         第1回のAudio変数を削除
TestScene.cpp       Audio::Load / Audio::Play を使う
```

Visual Studioのソリューションエクスプローラーで、`Engine/Audio.h` と `Engine/Audio.cpp` をプロジェクトに追加してください。

---

## 1. Engine/Audio.h を作成する

`Engine` フォルダに `Audio.h` を作成します。

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

    bool Load(const std::string& name, file_path& filepath);
    void Play(const std::string& name);
}
```


`file_path` は、ファイルパスを表す型として `std::filesystem::path` に別名を付けたものです。

```cpp
using file_path = std::filesystem::path;
```

これにより、呼び出し側では `L"Assets/..."` のようにワイド文字列を直接書かず、通常の文字列からパスを作って渡せます。

```cpp
file_path testPath = "Assets/Audio/test.wav";
Audio::Load("test", testPath);
```

今回の `Load` は `file_path&` なので、文字列を直接渡すのではなく、いったん `file_path` 変数を作ってから渡します。

この段階では、効果音とBGMはまだ分けません。
まずは「名前を付けて読み込む」「名前で鳴らす」だけを作ります。

---

## 2. Engine/Audio.cpp を作成する

`Engine` フォルダに `Audio.cpp` を作成します。

```cpp
#include "Audio.h"
#include <Audio.h>
#include <memory>
#include <unordered_map>

namespace
{
    std::unique_ptr<DirectX::AudioEngine> audioEngine_;
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> sounds_;
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
    sounds_.clear();
    audioEngine_.reset();
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
```

---

## 3. Main.cpp に Audio を組み込む

`Main.cpp` の include 部分に、次を追加します。

```cpp
#include "Engine\\Audio.h"
```

`Input::Initialize(hWnd);` の後に、Audioの初期化を追加します。

```cpp
Input::Initialize(hWnd); // 入力の初期化
Audio::Initialize();     // オーディオの初期化
```

メインループ内で、`pRootJob->UpdateSub();` の後に Audio の更新を追加します。

```cpp
pRootJob->UpdateSub();
Audio::Update();
```

終了処理に Audio の解放を追加します。

```cpp
Model::Release();
pRootJob->ReleaseSub();
Audio::Release();
Input::Release();
Direct3D::Release();
```

---

## 4. TestScene.h から第1回のAudio変数を削除する

第1回で `TestScene.h` に追加した次の include と変数は削除します。

```cpp
#include <memory>
#include <Audio.h>
```

```cpp
std::unique_ptr<DirectX::AudioEngine> audioEngine_;
std::unique_ptr<DirectX::SoundEffect> testSound_;
```

`TestScene` はDirectXTK Audioを直接知らない形に戻します。

---

## 5. TestScene.cpp で Audio を使う

`TestScene.cpp` の include に、次を追加します。

```cpp
#include "Engine/Audio.h"
```

`Initialize()` を次のように変更します。

```cpp
void TestScene::Initialize()
{
    Instantiate<Stage>(this);

    file_path testPath = "Assets/Audio/test.wav";
Audio::Load("test", testPath);
    Audio::Play("test");
}
```

`Update()` は、いったん空で構いません。

```cpp
void TestScene::Update()
{
}
```

`Release()` も、いったん空で構いません。

```cpp
void TestScene::Release()
{
}
```

---

## 6. ビルドして実行する

起動直後に `test.wav` が1回鳴れば成功です。

第1回と違う点は、`TestScene` がDirectXTK Audioを直接使っていないことです。

```text
第1回：TestSceneがDirectXTK Audioを直接使う
第2回：Audio機能の中でDirectXTK Audioを使う
```

---

## 7. スペースキーで音を鳴らす

`TestScene.cpp` の `Update()` を次のように変更します。

```cpp
void TestScene::Update()
{
    if (Input::IsKeyDown(DIK_SPACE))
    {
        Audio::Play("test");
    }
}
```

この変更で、スペースキーを押した瞬間に音が鳴ります。

`IsKeyDown` は押した瞬間だけ true になります。
押しっぱなしで何度も鳴らしたくないときに使います。

---

## 今回の完成状態

この回で、Audio機能が `Engine` 側に移動しました。

現在の使い方は次の形です。

```cpp
file_path testPath = "Assets/Audio/test.wav";
Audio::Load("test", testPath);
Audio::Play("test");
```

次回は、効果音とBGMを分けて管理できるようにします。
