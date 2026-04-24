# Step 1 ── ライト視点の行列を計算する

## 学習目標

- カメラ行列とライト行列は「見る場所・向きが違うだけで同じもの」だと理解する
- `XMMatrixLookAtLH` と `XMMatrixOrthographicLH` の使い方を知る
- まず**値を計算して ImGui に表示するだけ**にして、行列が何者かを体感する

---

## 理論：ライト視点の行列とは

通常の描画で使う `matWVP` は以下の3つの合成です。

```
matWVP = matWorld × matView × matProjection
                    ↑            ↑
              カメラの位置・向き  遠近感の設定
```

シャドウマップでは**カメラをライトに置き換えた行列**が必要です。

```
matLightVP = matLightView × matLightProjection
                ↑                  ↑
          ライトの位置・向き    平行光源 = 正射影（遠近なし）
```

### lightPosition の方向ベクトル規約

このプロジェクトでは `lightPosition` を **「サーフェスから光源へ向かう方向」** として扱います。  
diffuse の `L = normalize(lightPosition)` と同じ規約です。

```
lightPosition = (0.5, -1, 0.7)
      ↓
「この方向に光源がある」
      ↓
lightEye = normalize(lightPosition) * 10  ← 光源方向に仮想カメラを置く
```

> ⚠️ **negation 不要**：`-lightDir` にしてしまうと光源が逆方向に置かれ、影が逆側に出ます。

### 平行光源では「正射影」を使う理由

| 投影方式 | 使う場面 | 特徴 |
|---------|---------|------|
| 透視投影（Perspective） | カメラ・点光源 | 遠いほど小さく見える |
| 正射影（Orthographic） | **平行光源** | 距離に関係なく同じサイズ |

---

## 変更内容

### 変更ファイル：`Engine/Direct3D.h`

`GetLightViewMatrix()` と `GetLightProjectionMatrix()` の2つの関数を追加します。

**変更前（末尾付近）：**
```cpp
DirectX::XMFLOAT4 GetLightPos();
void SetLightPos(DirectX::XMFLOAT4 pos);
```

**変更後：**
```cpp
DirectX::XMFLOAT4 GetLightPos();
void SetLightPos(DirectX::XMFLOAT4 pos);

// シャドウマップ用：ライト視点の行列
DirectX::XMMATRIX GetLightViewMatrix();       // ライトのビュー行列
DirectX::XMMATRIX GetLightProjectionMatrix(); // ライトの正射影行列
```

---

### 変更ファイル：`Engine/Direct3D.cpp`

ファイル末尾（`SetLightPos` の下）に2つの関数を追加します。

```cpp
// ライトのビュー行列を返す
// lightPosition は「サーフェスから光源へ向かう方向ベクトル」（diffuse の L と同じ規約）
// lightEye は lightPosition 方向に置く（negation 不要）
DirectX::XMMATRIX Direct3D::GetLightViewMatrix()
{
    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat4(&lightPosition));
    XMVECTOR lightEye = lightDir * 10.0f;   // 光源方向に仮想ライト位置を置く
    XMVECTOR lightAt  = XMVectorSet(0, 0, 0, 0);

    // lightDir が Y 軸に平行なとき LookAt が破綻するので up を Z 軸に切り替える
    XMVECTOR upY = XMVectorSet(0, 1, 0, 0);
    float dotY   = fabsf(XMVectorGetX(XMVector3Dot(lightDir, upY)));
    XMVECTOR lightUp = (dotY > 0.99f) ? XMVectorSet(0, 0, 1, 0) : upY;

    return XMMatrixLookAtLH(lightEye, lightAt, lightUp);
}

// ライトの正射影行列を返す
// 平行光源なので XMMatrixOrthographicLH（透視投影ではない）
// width/height は部屋サイズに合わせて設定する（大きすぎると影が粗くなる）
DirectX::XMMATRIX Direct3D::GetLightProjectionMatrix()
{
    float width  =  5.0f; // 部屋サイズに合わせた値（20 より小さい方が影が細かい）
    float height =  5.0f;
    float nearZ  =  1.0f;
    float farZ   = 50.0f;

    return XMMatrixOrthographicLH(width, height, nearZ, farZ);
}
```

---

### 変更ファイル：`Stage.cpp`

`Stage::Draw()` の ImGui 表示部分に行列デバッグ表示を追加します。

```cpp
if (ImGui::CollapsingHeader("Light Matrix Debug"))
{
    XMMATRIX V  = Direct3D::GetLightViewMatrix();
    XMMATRIX P  = Direct3D::GetLightProjectionMatrix();
    XMMATRIX VP = V * P;

    ImGui::Text("-- LightView --");
    ImGui::Text("[0]: %.2f %.2f %.2f %.2f", V.r[0].m128_f32[0], ...);
    ...
}
```

---

## ✅ ここでビルドして実行

1. ビルドして実行する
2. ImGui の「Light Matrix Debug」を開いて行列の値が表示されれば成功
3. WASD / Up/Down でライト方向を変えると行列の値が変わることを確認する

---

## 次のステップ

[Step2 → シャドウマップ用テクスチャを作成する](./Step2_ShadowMapTexture.md)