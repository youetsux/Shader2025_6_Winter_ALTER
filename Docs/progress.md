# 進捗記録

## 現在の状態

**Step5 完了・コミット済み（`04f5ccd`）**

---

## 完了済み

### 事前準備（`8eee953`）
- `lightType_ = 0`（デフォルト：平行光源）
- ライト初期値を方向ベクトル `{ 0.5f, -1.0f, 0.7f, 0.0f }` に変更
- ImGui に平行光源 / 点光源の切り替えボタンを追加

### Step1：ライト視点の行列を計算する ✅（`0bf7ab5`）
- `Direct3D` に `GetLightViewMatrix()` / `GetLightProjectionMatrix()` を追加
- `Stage::Draw()` の ImGui に「Light Matrix Debug」折りたたみ表示を追加

### Step2：シャドウマップ用テクスチャを作成する ✅（`a83312b` に含む）
- `Direct3D` に `InitShadowMap()` / `GetShadowMapSRV()` を追加
- `namespace Direct3D` に `pShadowMapTexture` / `pShadowMapDSV` / `pShadowMapSRV` を追加
- `Initialize()` 内で `InitShadowMap(1024, 1024)` を呼ぶ
- `Release()` に `SAFE_RELEASE` を追加

### Step3：シャドウ用シェーダーを作成する ✅（`a83312b` に含む）
- `SHADER_SHADOWMAP` を enum に追加
- `ShadowMap.hlsl` を新規作成
- `InitShadowShader()` を追加、`InitShader()` から呼ぶ
- `BeginShadowPass()` / `EndShadowPass()` を追加

### Step4：2パス描画を組み込む ✅（`a83312b`）
- `Fbx` に `CB_SHADOW` / `pShadowConstantBuffer_` / `DrawShadow()` を追加
- `Model` に `DrawShadow()` を追加
- `Stage::Initialize()` に比較サンプラー作成を追加（`s1` スロット）
- `Stage::Draw()` を2パス構造に変更（`hRoom_` はシャドウキャスターに含めない）
- `CONSTANTBUFFER_STAGE` に `matLightVP` を追加
- `Stage::Update()` で `matLightVP` を計算・送信

### Step5：影の判定をシェーダーに追加する ✅（`04f5ccd`）
- `Simple3D.hlsl` に `g_shadowMap` / `g_shadowSampler` 宣言追加
- `cbuffer gStage` に `matLightVP` 追加
- `PS()` に影判定コードを追加（`SampleCmpLevelZero` + bias）
- `Stage::Draw()` に SRV のセット／解除を追加
- `GetLightViewMatrix()` の `lightEye` 方向を修正（negation 不要）
- `InitShadowShader()` のラスタライザーを `CULL_NONE` に修正
- `GetLightProjectionMatrix()` のフラスタムを `5.0f` に縮小

---

## 次にやること

### Step6：デバッグ UI とパラメータ調整
詳細は [`Step6_Debug.md`](./Step6_Debug.md) を参照。

主な変更：
- `Stage` に `shadowBias_` / `lightOrthoSize_` / `showShadowMap_` メンバを追加
- `GetLightProjectionMatrix(float orthoSize)` をオーバーロード
- ImGui にバイアスと正射影サイズのスライダー、シャドウマップ可視化を追加
- `HLSL` の固定 `bias` を `shadowBias` （CB 経由）に変更

---

## 再開方法

1. Visual Studio で `MyFirstGame.sln` を開く
2. このファイルで現在の進捗を確認する
3. 次のStepのドキュメントを開く
4. GitHub Copilot に「Step6実装して」と指示する
