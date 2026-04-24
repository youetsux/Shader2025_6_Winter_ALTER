# 進捗記録

## 現在の状況

**Step4 完了**

---

## 完了済み

### 準備作業
- `Simple3D.hlsl` をノーマル3Dシェーダーに戻した（`DrawToon` -> `Draw`）
- ImGui に平行光源 / 点光源の切り替えボタンを追加
- デフォルトを**平行光源**に変更（`lightType_ = 0`）
- `Direct3D.cpp` のライト初期値を方向ベクトルらしい値に変更（`{ 0.5f, -1.0f, 0.7f, 0.0f }`）

### Step1：ライト視点の行列を計算する [完了]
- `Engine/Direct3D.h` に `GetLightViewMatrix()` / `GetLightProjectionMatrix()` を追加
- `Engine/Direct3D.cpp` に上記2関数を実装（`XMMatrixLookAtLH` / `XMMatrixOrthographicLH`）
- `Stage.cpp` の ImGui に「Light Matrix Debug」折りたたみ表示を追加

### Step2：シャドウマップ用テクスチャを作成する [完了]
- `Engine/Direct3D.h` に `InitShadowMap()` / `GetShadowMapSRV()` の宣言を追加
- `Engine/Direct3D.cpp` の namespace 内にリソース変数3つ（Texture2D / DSV / SRV）を追加
- `Engine/Direct3D.cpp` に `InitShadowMap()` / `GetShadowMapSRV()` を実装
- `Engine/Direct3D.cpp` の `Initialize()` 内で `InitShadowMap(1024, 1024)` を呼ぶ
- `Engine/Direct3D.cpp` の `Release()` に `SAFE_RELEASE` 3つを追加
- `Stage.cpp` の ImGui に「ShadowMap SRV: OK/null」確認表示を追加

### Step3：シャドウ用シェーダーを作成する [完了]
- `ShadowMap.hlsl` を新規作成（ライト視点の深度書き込み専用シェーダー）
- `Engine/Direct3D.h` の `SHADER_TYPE` 列挙型に `SHADER_SHADOWMAP` を追加
- `Engine/Direct3D.h` に `InitShadowShader()` の宣言を追加
- `Engine/Direct3D.cpp` に `InitShadowShader()` の実装を追加
- `Engine/Direct3D.cpp` の `InitShader()` 内から `InitShadowShader()` を呼ぶ

### Step4：2パス描画を組み込む [完了]
- `Engine/Direct3D.h` に `BeginShadowPass()` / `EndShadowPass()` の宣言を追加
- `Engine/Direct3D.cpp` に `screenWidth` / `screenHeight` 変数を追加
- `Engine/Direct3D.cpp` の `Initialize()` で画面サイズを保存
- `Engine/Direct3D.cpp` に `BeginShadowPass()` / `EndShadowPass()` を実装
- `Engine/Fbx.h` に `CB_SHADOW` 構造体・`pShadowConstantBuffer_` メンバ・`DrawShadow()` 宣言を追加
- `Engine/Fbx.cpp` のコンストラクタに `pShadowConstantBuffer_(nullptr)` を追加
- `Engine/Fbx.cpp` の `InitConstantBuffer()` でシャドウ用CBを作成
- `Engine/Fbx.cpp` に `DrawShadow()` を実装
- `Engine/Model.h` / `Model.cpp` に `DrawShadow()` を追加
- `Stage.cpp` の `Draw()` を2パス構造に変更
- キャスター（ドーナツのみ）・レシーバー（部屋）のコメント整理

---

## 次にやること

### Step5：影の判定をシェーダーに追加する（影が出る！）
詳細は `Docs/Step5_ShadowReceive.md` を参照。

---

## 再開方法

1. Visual Studio で `MyFirstGame.sln` を開く
2. このファイル（`Docs/progress.md`）で現在の進捗を確認する
3. 該当Stepのドキュメント（上記「次にやること」）を開く
4. GitHub Copilot に「Step5を進めて」と指示する

---

## 注意事項

- **Stepは1つずつ・ビルド確認・コミットの順で進める**
- 各Stepのドキュメントに「ここでビルドして実行」のチェックポイントがある