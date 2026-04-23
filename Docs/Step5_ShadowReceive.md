# Step 5 ── 影の判定をシェーダーに追加する（影が出る！）

## 学習目標

- ワールド座標をライト視点のクリップ座標に変換する手順を理解する
- 「シャドウマップの深度値」と「現在のピクセルの深度値」を比較する方法を知る
- `SampleCmpLevelZero` と比較サンプラーの仕組みを理解する
- シャドウバイアスがなぜ必要かを体感する

---

## 理論：影の判定はどうやるのか

パス1でシャドウマップ（ライト視点の深度テクスチャ）が完成しました。  
パス2（メインパス）のピクセルシェーダーでは以下の4ステップで影を判定します。

```
① このピクセルのワールド座標を「ライト視点のクリップ空間」に変換する
       ↓
② クリップ座標 → シャドウマップの UV 座標に変換する
       ↓
③ シャドウマップをサンプリングして「ライトから見た深度値」を取得する
       ↓
④ 現在のピクセルの深度値と比較する
       ライト深度 ≒ 現在深度 → ライトから見えていた = 明るい
       ライト深度 <  現在深度 → 何かに遮られていた = 影
```

### クリップ座標から UV 座標への変換

クリップ空間は `-1〜+1`、UV 空間は `0〜1` です。変換式は以下です。

```
u = clipPos.x / clipPos.w * 0.5 + 0.5
v = clipPos.y / clipPos.w * (-0.5) + 0.5  ← Y軸は反転
```

Y 軸を反転する理由：  
DirectX のクリップ空間は上が `+1`、UV 空間は上が `0` と逆向きのためです。

---

## 変更内容

### 変更ファイル：`Stage.h`

`CONSTANTBUFFER_STAGE` に `matLightVP` を追加します。

**変更前：**
```cpp
struct CONSTANTBUFFER_STAGE
{
    XMFLOAT4 lightPosition;
    XMFLOAT4 eyePosition;
    int      lightType;
    XMFLOAT3 _pad;
};
```

**変更後：**
```cpp
struct CONSTANTBUFFER_STAGE
{
    XMFLOAT4            lightPosition;  // 光源の方向 or 位置
    XMFLOAT4            eyePosition;    // カメラ位置
    int                 lightType;      // 0=平行光源, 1=点光源
    XMFLOAT3            _pad;           // アライメント用（Step6 でさらに変更します）
    XMFLOAT4X4          matLightVP;     // ライト視点の View×Projection 行列（影判定用）
};
```

> **注意：** Step6 で `shadowBias` を追加する際にこの struct をもう一度変更します。  
> Step5 ではここに書いた形のまま進めてください。

---

### 変更ファイル：`Stage.cpp`

`Stage::Update()` のコンスタントバッファ送信部分に `matLightVP` を追加します。

**変更前（一部）：**
```cpp
CONSTANTBUFFER_STAGE cb;
cb.lightPosition = Direct3D::GetLightPos();
XMStoreFloat4(&cb.eyePosition, Camera::GetPosition());
cb.lightType = lightType_;
cb._pad = { 0, 0, 0 };
```

**変更後：**
```cpp
CONSTANTBUFFER_STAGE cb;
cb.lightPosition = Direct3D::GetLightPos();
XMStoreFloat4(&cb.eyePosition, Camera::GetPosition());
cb.lightType = lightType_;
cb._pad = { 0, 0, 0 };

// ライト視点の VP 行列を計算して送信
XMMATRIX lightV  = Direct3D::GetLightViewMatrix();
XMMATRIX lightP  = Direct3D::GetLightProjectionMatrix();
XMMATRIX lightVP = lightV * lightP;
XMStoreFloat4x4(&cb.matLightVP, XMMatrixTranspose(lightVP));
```

また、`Stage::Draw()` のメインパス部分でシャドウマップ SRV を  
ピクセルシェーダーのテクスチャスロット `t1` にセットします。

```cpp
// パス2 開始前に追記
ID3D11ShaderResourceView* pShadowSRV = Direct3D::GetShadowMapSRV();
Direct3D::pContext->PSSetShaderResources(1, 1, &pShadowSRV);
```

また描画終了後に SRV を解除します（DSV と SRV を同時にバインドするとエラーになるため）。

```cpp
// パス2 終了後に追記（次のフレームのパス1 前に SRV を外す）
ID3D11ShaderResourceView* nullSRV = nullptr;
Direct3D::pContext->PSSetShaderResources(1, 1, &nullSRV);
```

---

### 変更ファイル：`Simple3D.hlsl`

#### ① 比較サンプラーとシャドウマップテクスチャを追加

ファイル先頭のテクスチャ宣言部分に追記します。

**変更前：**
```hlsl
Texture2D    g_texture : register(t0);
SamplerState g_sampler : register(s0);
```

**変更後：**
```hlsl
Texture2D               g_texture      : register(t0); // モデルのテクスチャ
SamplerState            g_sampler      : register(s0); // 通常サンプラー

Texture2D               g_shadowMap    : register(t1); // シャドウマップ（深度テクスチャ）
SamplerComparisonState  g_shadowSampler: register(s1); // 比較サンプラー
```

#### ② `cbuffer gStage` に `matLightVP` を追加

**変更前：**
```hlsl
cbuffer gStage : register(b1)
{
    float4 lightPosition;
    float4 eyePosition;
    int    lightType;
    float3 _pad;
};
```

**変更後：**
```hlsl
cbuffer gStage : register(b1)
{
    float4            lightPosition;
    float4            eyePosition;
    int               lightType;
    float3            _pad;
    row_major float4x4 matLightVP;  // ライト視点の VP 行列
};
```

#### ③ ピクセルシェーダーに影判定を追加

`PS()` 関数の最後の `return color;` の直前に追記します。

```hlsl
// ========== シャドウ判定 ==========
float shadow = 1.0; // デフォルト: 影なし（明るい）

// ① ワールド座標をライト視点のクリップ空間に変換
float4 lightClipPos = mul(inData.wpos, matLightVP);

// ② クリップ座標 → UV 座標に変換
// クリップ空間は -1〜+1、UV 空間は 0〜1
float2 shadowUV;
shadowUV.x =  lightClipPos.x / lightClipPos.w * 0.5 + 0.5;
shadowUV.y = -lightClipPos.y / lightClipPos.w * 0.5 + 0.5; // Y軸反転

// ③ UV が 0〜1 の範囲内にある場合のみ判定（範囲外 = ライトの視野外）
if (shadowUV.x >= 0.0 && shadowUV.x <= 1.0 &&
    shadowUV.y >= 0.0 && shadowUV.y <= 1.0)
{
    // ④ 現在のピクセルの深度値
    float currentDepth = lightClipPos.z / lightClipPos.w;

    // ⑤ シャドウバイアス（シャドウアクネ防止のため少し手前を参照）
    float bias = 0.005;

    // ⑥ 比較サンプラーで深度を比較
    // SampleCmpLevelZero: シャドウマップの値 >= (currentDepth - bias) なら 1.0、そうでなければ 0.0
    shadow = g_shadowMap.SampleCmpLevelZero(
        g_shadowSampler, shadowUV, currentDepth - bias);
}

// ⑦ 影の適用（影の中は 30% の明るさ）
color *= (0.3 + 0.7 * shadow);
// ========== シャドウ判定 END ==========

return color;
```

#### ④ 比較サンプラーの設定（`Stage.cpp` で追加）

`SamplerComparisonState` は CPU 側でも設定が必要です。  
`Stage::Initialize()` または `Direct3D::Initialize()` に以下を追加します。

```cpp
// 比較サンプラー（シャドウマップ用）
D3D11_SAMPLER_DESC sampDesc = {};
sampDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.BorderColor[0] = 1.0f; // 範囲外は「影なし」扱い
sampDesc.BorderColor[1] = 1.0f;
sampDesc.BorderColor[2] = 1.0f;
sampDesc.BorderColor[3] = 1.0f;
sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
sampDesc.MinLOD         = 0;
sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;

ID3D11SamplerState* pShadowSampler = nullptr;
Direct3D::pDevice->CreateSamplerState(&sampDesc, &pShadowSampler);
Direct3D::pContext->PSSetSamplers(1, 1, &pShadowSampler);
```

---

## ✅ ここでビルドして実行

1. ビルドして実行する
2. **モデルに影が表示されれば成功！**
3. もし影が全面真っ黒になる場合は `bias` の値を大きくしてみてください（例：`0.01`）
4. 影のエッジがギザギザな場合は Step6 でPCFフィルタリングを追加します

> **ポイント：** `shadow = 0.3 + 0.7 * shadow` の意味は  
> 「影の中でも 30% の明るさは残す」です。  
> 完全に真っ暗（`shadow * color`）にすると不自然に見えます。

---

## よくある疑問

**Q. シャドウアクネとは何？**  
A. バイアスなしだと、自分自身の深度値とシャドウマップの深度値が  
　わずかにずれて「自分自身に影を落とす」縞模様のノイズが発生します。  
　これをシャドウアクネといい、バイアスを加えることで防ぎます。

**Q. `SampleCmpLevelZero` と普通の `Sample` の違いは？**  
A. 普通の `Sample` は色や深度の値を取得するだけですが、  
　`SampleCmpLevelZero` は「取得した値と比較値を比べて 0.0 か 1.0 を返す」関数です。  
　さらに `COMPARISON_MIN_MAG_LINEAR` のフィルタを使うと  
　周囲4テクセルの比較結果を補間して柔らかい影のエッジが得られます（PCF）。

**Q. `AddressMode = BORDER` で `BorderColor = 1.0` にするのはなぜ？**  
A. シャドウマップの範囲外（UV が 0〜1 を超えた場所）を「影なし」にするためです。  
　`1.0` は「深度最大値 = 何も遮っていない」を意味します。

---

## 次のステップ

[Step6 → デバッグ UI とパラメータ調整](./Step6_Debug.md)
