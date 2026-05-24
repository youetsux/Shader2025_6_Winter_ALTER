# 第6章：描画を2パス構成にする

## この章でやること

**`Stage::Draw()` の描画を2回に分けます。**

```text
パス1：ライト視点で深度だけ描く（シャドウパス）
パス2：カメラ視点で普通に描く（メインパス）
```

あわせて、第7章でシェーダーがライトVP行列を使えるよう、  
`CONSTANTBUFFER_STAGE` に `matLightVP` を追加して毎フレーム送ります。

この章が終わっても**画面の見た目は変わりません**。  
影が出るのは第7章からです。

---

## この章でのコード変更点

| ファイル | 変更内容 |
|---|---|
| `Stage.h` | `CONSTANTBUFFER_STAGE` に `matLightVP` を追加 |
| `Stage.cpp` | `Update()` でライトVP行列を計算してCBに入れる |
| `Stage.cpp` | `Draw()` を2パス構成に変える |

---

## ① `matLightVP` を追加する

### ここでは何をするか

`CONSTANTBUFFER_STAGE`（シェーダーの `cbuffer gStage` に対応）に、  
ライト視点のVP行列 `matLightVP` を追加します。

### なぜ追加するのか

第7章でシェーダーが影を判定するとき、「このピクセルはシャドウマップ上のどこにあるか」を計算するためにライトVP行列が必要です。  
今章ではまだ使いませんが、**データだけ先に送れる状態にしておきます**。

```cpp
// Before（Stage.h）
struct CONSTANTBUFFER_STAGE
{
    XMFLOAT4 lightPosition;
    XMFLOAT4 eyePosition;
    int      lightType;
    XMFLOAT3 _pad;
    // matLightVP がない
};

// After（Stage.h）
struct CONSTANTBUFFER_STAGE
{
    XMFLOAT4   lightPosition;
    XMFLOAT4   eyePosition;
    int        lightType;
    XMFLOAT3   _pad;
    XMFLOAT4X4 matLightVP;  // ← 追加
};
```

---

## ② Update() でライトVP行列を計算する

### ここでは何をするか

毎フレーム、ライトのビュー行列とプロジェクション行列を掛け合わせて `matLightVP` を作り、コンスタントバッファに入れます。

```cpp
// After（Stage.cpp の Update() 内、cbに値をセットする部分）
XMMATRIX lightV  = Direct3D::GetLightViewMatrix();
XMMATRIX lightP  = Direct3D::GetLightProjectionMatrix();
XMMATRIX lightVP = lightV * lightP;
XMStoreFloat4x4(&cb.matLightVP, lightVP);
// row_major 指定のため、転置（XMMatrixTranspose）は不要
```

---

## ③ Draw() を2パスに分ける

### ここでは何をするか

今まで1回だった `Draw()` を、シャドウパスとメインパスの2回に分けます。

### なぜ部屋（hRoom_）はシャドウパスに入れないのか

部屋の外壁がライトを遮ってしまい、室内全体が影になる可能性があるためです。  
影を落とすのはドーナツだけにします。

```text
影を落とすもの：ドーナツ（hDonut_）
影を受けるもの：部屋・床（hRoom_）
```

```cpp
// Before（Stage.cpp）
void Stage::Draw()
{
    Model::Draw(hball_);
    Model::Draw(hRoom_);
    Model::Draw(hDonut_);
}

// After（Stage.cpp）
void Stage::Draw()
{
    // ===== パス1：シャドウパス =====
    // ライト視点でドーナツの深度だけ書く
    Direct3D::BeginShadowPass();
    Model::DrawShadow(hDonut_);
    Direct3D::EndShadowPass();

    // ===== パス2：メインパス =====
    // 普通にカメラ視点で全部描く
    Model::Draw(hball_);
    Model::Draw(hRoom_);
    Model::Draw(hDonut_);
}
```

---

## 学生向け説明

今までは1フレームに1回だけ描いていた。

```text
カメラから見て描く
```

シャドウマップでは、先にライトから見たZ値を作る必要がある。

```text
1回目：ライトから見て、深度（Z値）だけ保存する
2回目：カメラから見て、普通に描く
```

この「1フレームに2回描く」ことを**2パス描画**という。

---

## ビルド確認

- ビルドが通れば成功。
- 画面はほぼ変わらない。
- まだ影が出なくても正常。

---

## 推奨コミット

```bash
git add Stage.h Stage.cpp
git commit -m "chapter6: Draw を2パス構成にする"
```
