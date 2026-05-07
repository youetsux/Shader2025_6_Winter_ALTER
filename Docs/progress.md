# 進捗記録

## 現在の状況

**Step5 完了**

---

## 完了済み

### 準備作業
- `Simple3D.hlsl` をノーマル3Dシェーダーに戻した
- ImGui に平行光源 / 点光源の切り替えボタンを追加
- デフォルトを**平行光源**に変更（`lightType_ = 0`）
- `Direct3D.cpp` のライト初期値を方向ベクトルらしい値に変更（`{ 0.5f, -1.0f, 0.7f, 0.0f }`）

### Step1：ライト視点の行列を計算する [完了]
- `Engine/Direct3D.h` に `GetLightViewMatrix()` / `GetLightProjectionMatrix()` を追加
- `Engine/Direct3D.cpp` に上記2関数を実装
- `Stage.cpp` の ImGui に「Light Matrix Debug」折りたたみ表示を追加

### Step2：シャドウマップ用テクスチャを作成する [完了]
- `Engine/Direct3D.h` / `cpp` に `InitShadowMap()` / `GetShadowMapSRV()` を実装
- `Stage.cpp` の ImGui に「ShadowMap SRV: OK/null」確認表示を追加

### Step3：シャドウ用シェーダーを作成する [完了]
- `ShadowMap.hlsl` を新規作成
- `Engine/Direct3D` に `SHADER_SHADOWMAP` / `InitShadowShader()` を追加

### Step4：2パス描画を組み込む [完了]
- `Engine/Direct3D` に `BeginShadowPass()` / `EndShadowPass()` を実装
- `Engine/Fbx` に `DrawShadow()` を追加
- `Engine/Model` に `DrawShadow()` を追加
- `Stage.cpp` の `Draw()` を2パス構造に変更
- 部屋はレシーバーのみ（シャドウパスには含めない）コメント整理

### Step5：影の判定をシェーダーに追加する [完了]
- `Stage.h`：`CONSTANTBUFFER_STAGE` に `matLightVP` を追加
- `Stage.cpp Update()`：`matLightVP` を計算・送信
- `Stage.cpp Draw()`：SRV をスロット `t1` にセット、パス2終了後に解除
- `Stage.cpp Initialize()`：比較サンプラー（`LESS_EQUAL`）を作成
- `Simple3D.hlsl`：シャドウマップ参照・`cbuffer` 追加・影判定ロジック追加
- `Engine/Direct3D.cpp` 修正（Step5 デバッグ中に発覚した問題）：
  - `GetLightViewMatrix()`：`lightEye = +lightDir * 10`（negation 不要・Y軸縮退バグ修正も）
  - `InitShadowShader()`：`CULL_FRONT` → `CULL_NONE`（背面深度誤判定を修正）
  - `GetLightProjectionMatrix()`：フラスタム `20x20` → `5x5`（影の解像度改善）
  - バイアス `bias = 0.005`（シャドウアクネ抑制）

---

## 次にやること

### Step6：デバッグ UI とパラメータ調整
詳細は `Docs/Step6_Debug.md` を参照。

---

## 再開方法

1. Visual Studio で `MyFirstGame.sln` を開く
2. このファイル（`Docs/progress.md`）で現在の進捗を確認する
3. 該当 Step のドキュメントを開く
4. GitHub Copilot に「Step6 を進めて」と指示する

---

## 注意事項

- **Step は1つずつ・ビルド確認・コミットの順で進める**
- `lightPosition` の規約：**サーフェスから光源へ向かう方向ベクトル**（diffuse の L と同じ）
- `lightEye = lightDir * 10.0f`（negation 不要）