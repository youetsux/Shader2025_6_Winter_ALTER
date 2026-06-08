# 第1回 DirectXTK Audioで音を1回鳴らす

## 目標

この回では、DirectXTK Audioを使って、WAVファイルを1回再生します。
まだオーディオエンジン化はしません。
まずは「DirectXTK Audioで音が鳴る」ことを確認します。

## 前提

DirectXTKのNuGet設定は完了しているものとします。
この資料では、NuGet設定の手順は扱いません。

使用する音声ファイルを次の場所に用意してください。

```text
Assets/Audio/test.wav
```

WAVファイル名は、必ず `test.wav` にしてください。
ファイル名やフォルダ名が違うと読み込みに失敗します。

---

## 今回変更するファイル

```text
TestScene.h
TestScene.cpp
```

今回は `Main.cpp` にはまだ手を入れません。
DirectXTK Audioの最小コードを `TestScene` に直接書いて確認します。

---

## 1. TestScene.h にAudio用の変数を追加する

`TestScene.h` を開きます。

先頭付近に、次の include を追加します。

```cpp
#include <memory>
#include <filesystem>
#include <Audio.h>
```

次に、`TestScene` クラスの private メンバとして、次の2つを追加します。

```cpp
private:
    std::unique_ptr<DirectX::AudioEngine> audioEngine_;
    std::unique_ptr<DirectX::SoundEffect> testSound_;
```

`TestScene.h` 全体の形は、だいたい次のようになります。

```cpp
#pragma once
#include "Engine/GameObject.h"
#include <memory>
#include <filesystem>
#include <Audio.h>

class TestScene : public GameObject
{
public:
    TestScene(GameObject* parent);
    ~TestScene();

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void Release() override;

private:
    std::unique_ptr<DirectX::AudioEngine> audioEngine_;
    std::unique_ptr<DirectX::SoundEffect> testSound_;
};
```

---

## 2. TestScene.cpp の Initialize で音声を読み込む

`TestScene.cpp` の `Initialize()` に処理を追加します。

変更前は次のようになっています。

```cpp
void TestScene::Initialize()
{
    Instantiate<Stage>(this);
}
```

次のように変更します。

```cpp
void TestScene::Initialize()
{
    Instantiate<Stage>(this);

    std::filesystem::path filepath = "Assets/Audio/test.wav";

    audioEngine_ = std::make_unique<DirectX::AudioEngine>();
    testSound_ = std::make_unique<DirectX::SoundEffect>(
        audioEngine_.get(),
        filepath.c_str()
    );

    testSound_->Play();
}
```

ここで行っていることは、次の3つです。

```text
AudioEngineを作る
WAVファイルを読み込む
Playで再生する
```

`AudioEngine` は音を鳴らす仕組み本体です。
`SoundEffect` は音声ファイル1個分のデータです。

---

## 3. ファイルパスの型を確認する

今回のコードでは、音声ファイルの場所を `std::filesystem::path` に入れています。

```cpp
std::filesystem::path filepath = "Assets/Audio/test.wav";
```

`std::filesystem::path` は、ファイルやフォルダの場所を表すための型です。
文字列のまま扱うよりも、「これはファイルパスである」という意味が分かりやすくなります。

DirectXTK Audio の `SoundEffect` に渡すときは、`filepath.c_str()` を使います。

```cpp
testSound_ = std::make_unique<DirectX::SoundEffect>(
    audioEngine_.get(),
    filepath.c_str()
);
```

第2回から作る `Audio.h` では、次のように短い名前を付けて使います。

```cpp
using file_path = std::filesystem::path;
```

---

## 4. スマートポインタの意味を確認する

今回のコードでは、次のような書き方をしています。

```cpp
audioEngine_ = std::make_unique<DirectX::AudioEngine>();
```

これは、簡単に言うと次の意味です。

```text
AudioEngineをnewで作る
作ったものをunique_ptrに持たせる
使い終わったら自動でdeleteしてもらう
```

`std::unique_ptr` は、作ったオブジェクトを1か所だけで管理するためのスマートポインタです。
普通のポインタと違って、使い終わったときに自動で解放してくれます。

生ポインタで書くと、次のようになります。

```cpp
DirectX::AudioEngine* audioEngine_ = nullptr;
DirectX::SoundEffect* testSound_ = nullptr;

audioEngine_ = new DirectX::AudioEngine();

testSound_ = new DirectX::SoundEffect(
    audioEngine_,
    filepath.c_str()
);
```

この場合、終了時に自分で `delete` する必要があります。

```cpp
delete testSound_;
testSound_ = nullptr;

delete audioEngine_;
audioEngine_ = nullptr;
```

消す順番は、作った順番の逆です。

```text
作る順番：AudioEngine → SoundEffect
消す順番：SoundEffect → AudioEngine
```

今回の授業では、解放忘れを防ぐために `std::unique_ptr` を使います。
`std::make_unique` は、`new` と `unique_ptr` への代入をまとめて行う便利な書き方です。

`SoundEffect` を作るときの `audioEngine_.get()` は、`unique_ptr` の中に入っているポインタを一時的に取り出すための書き方です。

```cpp
testSound_ = std::make_unique<DirectX::SoundEffect>(
    audioEngine_.get(),
    filepath.c_str()
);
```

ここでは、`SoundEffect` を作るために `AudioEngine` の場所を渡しています。
`get()` で取り出したポインタを `delete` してはいけません。
解放は `unique_ptr` に任せます。

---

## 5. UpdateでAudioEngineを更新する

`TestScene.cpp` の `Update()` に、次の処理を追加します。

```cpp
void TestScene::Update()
{
    if (audioEngine_ != nullptr)
    {
        audioEngine_->Update();
    }
}
```

`AudioEngine` は毎フレーム更新します。
今後、Audio処理をエンジン側に移したあとも、`Update()` は必要になります。

---

## 6. Releaseで解放する

`TestScene.cpp` の `Release()` に次の処理を追加します。

```cpp
void TestScene::Release()
{
    testSound_.reset();
    audioEngine_.reset();
}
```

`unique_ptr` を使っているので、`reset()` で解放できます。

---

## 7. ビルドして実行する

実行したときに、起動直後に `test.wav` が1回鳴れば成功です。

確認すること：

```text
ビルドが通る
起動直後に音が鳴る
終了時にエラーが出ない
```

---

## よくある失敗

### Audio.h が見つからない

DirectXTKのNuGet設定ができていない可能性があります。

```cpp
#include <Audio.h>
```

でエラーが出る場合は、DirectXTKがプロジェクトに追加されているか確認してください。

### 音が鳴らない

まずファイルパスを確認してください。

```cpp
"Assets/Audio/test.wav"
```

実行時の作業フォルダから見て、この場所にファイルが必要です。

### mp3やoggを指定している

今回使うファイルはWAVです。
まずはWAVファイルで確認してください。

---

## 今回の完成状態

この回では、音声処理をまだ `TestScene` に直接書いています。
これは最終形ではありません。

次回は、音声処理を `Engine/Audio.h` と `Engine/Audio.cpp` に移して、ゲームエンジンの機能として使える形にします。
