# 進捗記録

## 現在の状態

**Step1 完了・コミット済み**

---

## 完了済み

### 事前準備
- `Simple3D.hlsl` をノーマル3Dシェーダーに戻した（`DrawToon` → `Draw`）
- ImGui に平行光源 / 点光源の切り替えボタンを追加
- デフォルトを**平行光源**に変更（`lightType_ = 0`）
- `Direct3D.cpp` のライト初期値を方向ベクトルらしい値に変更（`{ 0.5f, -1.0f, 0.7f, 0.0f }`）

### Step1：ライト視点の行列を計算する ✅
- `Engine/Direct3D.h` に `GetLightViewMatrix()` / `GetLightProjectionMatrix()` を追加
- `Engine/Direct3D.cpp` に上記2関数を実装（`XMMatrixLookAtLH` / `XMMatrixOrthographicLH`）
- `Stage.cpp` の ImGui に「Light Matrix Debug」折りたたみ表示を追加

---

## 次にやること

### Step2：シャドウマップ用テクスチャを作成する
詳細は [`Step2_ShadowMapTexture.md`](./Step2_ShadowMapTexture.md) を参照。

---

## 再開方法

1. Visual Studio で `MyFirstGame.sln` を開く
2. このファイル（`Docs/progress.md`）で現在の進捗を確認する
3. 次のStepのドキュメント（上記「次にやること」）を開く
4. GitHub Copilot に「Step2実装して」と指示する

---

## 注意事項

- **Stepは1つずつ実装・ビルド確認・コミットの順で進める**
- 各Stepのドキュメントに「✅ ここでビルドして実行」のチェックポイントがある
- Step4は A〜D の4段階に分かれている（それぞれビルド確認あり）
