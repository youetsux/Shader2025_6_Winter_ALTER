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

### 平行光源では「正射影」を使う理由

| 投影方式 | 使う場面 | 特徴 |
|---------|---------|------|
| 透視投影（Perspective） | カメラ・点光源 | 遠いほど小さく見える |
| 正射影（Orthographic） | **平行光源** | 距離に関係なく同じサイズ |

太陽光のような平行光源は「無限遠から平行に降り注ぐ」ため、  
距離による拡大縮小が起きない正射影が正しい選択です。

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
// ・lightPosition を「光が来る方向」として、逆方向に仮想的な光源位置を置く
// ・注視点はシーンの中心（0, 0, 0）
DirectX::XMMATRIX Direct3D::GetLightViewMatrix()
{
    // lightPosition は「光の方向ベクトル」なので、
    // 逆方向に少し離れた場所に「仮想ライト位置」を置く
    XMVECTOR lightDir = XMLoadFloat4(&lightPosition);
    XMVECTOR lightEye = -XMVector3Normalize(lightDir) * 10.0f; // 原点から10離れた位置
    XMVECTOR lightAt  = XMVectorSet(0, 0, 0, 0);               // 注視点：シーンの中心
    XMVECTOR lightUp  = XMVectorSet(0, 1, 0, 0);               // 上方向

    return XMMatrixLookAtLH(lightEye, lightAt, lightUp);
}

// ライトの正射影行列を返す
// ・平行光源なので XMMatrixOrthographicLH（透視投影ではない）
// ・幅・高さはシーン全体をカバーできるサイズに設定する
DirectX::XMMATRIX Direct3D::GetLightProjectionMatrix()
{
    float width  = 20.0f; // シーンをカバーする幅
    float height = 20.0f; // シーンをカバーする高さ
    float nearZ  =  1.0f; // 近クリップ面
    float farZ   = 50.0f; // 遠クリップ面

    return XMMatrixOrthographicLH(width, height, nearZ, farZ);
}
```

---

### 変更ファイル：`Stage.cpp`

`Stage::Draw()` の ImGui 表示部分に、行列の内容をデバッグ表示するコードを追加します。

```cpp
// ========== Step1 デバッグ：ライト行列を表示 ==========
if (ImGui::CollapsingHeader("Light Matrix Debug"))
{
    XMMATRIX V = Direct3D::GetLightViewMatrix();
    XMMATRIX P = Direct3D::GetLightProjectionMatrix();
    XMMATRIX VP = V * P;

    ImGui::Text("LightView[0]: %.2f %.2f %.2f %.2f",
        V.r[0].m128_f32[0], V.r[0].m128_f32[1], V.r[0].m128_f32[2], V.r[0].m128_f32[3]);
    ImGui::Text("LightView[1]: %.2f %.2f %.2f %.2f",
        V.r[1].m128_f32[0], V.r[1].m128_f32[1], V.r[1].m128_f32[2], V.r[1].m128_f32[3]);
    ImGui::Text("LightView[2]: %.2f %.2f %.2f %.2f",
        V.r[2].m128_f32[0], V.r[2].m128_f32[1], V.r[2].m128_f32[2], V.r[2].m128_f32[3]);
    ImGui::Text("LightView[3]: %.2f %.2f %.2f %.2f",
        V.r[3].m128_f32[0], V.r[3].m128_f32[1], V.r[3].m128_f32[2], V.r[3].m128_f32[3]);
}
// ========== Step1 デバッグ END ==========
```

---

## ✅ ここでビルドして実行

1. ビルドして実行する
2. ImGui ウィンドウの **「Light Matrix Debug」** をクリックして展開する
3. `LightView` の4行4列の数値が表示されれば成功
4. WASD でライト方向を動かすと行列の数値がリアルタイムに変わることを確認する

> **ポイント：** 見た目は変わりません。行列の数値が変わることだけ確認しましょう。  
> これが「ライトの向きに連動してビュー行列が変わる」証拠です。

---

## よくある疑問

**Q. なぜ `lightPosition` を「位置」ではなく「方向」として使うの？**  
A. 平行光源は「無限遠から来る平行な光」なので位置に意味がありません。  
　方向ベクトルをそのまま使う方が直感的です。  
　仮想的な位置は `GetLightViewMatrix()` 内で自動計算します。

**Q. `XMMatrixOrthographicLH` の幅・高さはどう決める？**  
A. シーン全体が収まるサイズにします。小さすぎると影が欠け、大きすぎると精度が落ちます。  
　Step6 で ImGui スライダーで調整できるようにします。

---

## 次のステップ

[Step2 → シャドウマップ用テクスチャを作成する](./Step2_ShadowMapTexture.md)
