# Step 5 ── 影の判定をシェーダーに追加する（影が出る！）

## 学習目標

- ワールド座標をライト視点のクリップ座標に変換する手順を理解する
- 「シャドウマップの深度値」と「現在のピクセルの深度値」を比較する方法を知る
- `SampleCmpLevelZero` と `LESS_EQUAL` 比較サンプラーの仕組みを理解する
- シャドウバイアスとシャドウアクネがなぜ発生するか体感する

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
       (currentDepth - bias) <= shadowMap → ライトから見えていた = 明るい（shadow=1）
       (currentDepth - bias) >  shadowMap → 何かに遮られていた = 影（shadow=0）
```

### クリップ座標から UV 座標への変換

```
u =  clipPos.x / clipPos.w * 0.5 + 0.5
v = -clipPos.y / clipPos.w * 0.5 + 0.5  ← Y軸は反転（DirectX の UV 規約）
```

### LESS_EQUAL 比較とボーダーカラーの関係

シャドウマップはフレームの最初に `1.0`（最大深度）でクリアします。

| ピクセルの状況 | shadowMap 値 | compareValue | LESS_EQUAL | 結果 |
|---|---|---|---|---|
| 影（何かが遮っている） | 遮蔽物の深度（小） | currentDepth - bias（大） | NO | shadow=0（暗い）|
| 明るい（未書き込み） | 1.0（クリア値） | currentDepth - bias（< 1.0） | YES | shadow=1（明るい）|

### シャドウアクネとバイアス

`CULL_NONE` を使うと前面深度が正確に書き込まれますが、  
浮動小数点誤差でドーナツが自分自身に影を落とす「シャドウアクネ」が発生します。  
`bias = 0.005` を引くことで自己シャドウを防ぎます。

```
compareValue = currentDepth - bias
```

> bias が大きすぎると影が消える。ドーナツ〜床の深度差（≈ 0.007）を超えないこと。

---

## 実装でハマったポイント（後輩への注意）

### ① lightEye の方向は negation 不要

`lightPosition` は「サーフェスから光源へ向かう方向」なので：

```cpp
// ✅ 正しい
XMVECTOR lightEye = lightDir * 10.0f;

// ❌ 間違い（影が逆に出る）
XMVECTOR lightEye = -lightDir * 10.0f;
```

### ② 比較関数は LESS_EQUAL

```cpp
sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
```

`SampleCmpLevelZero` は `(compareValue <= shadowMap)` のとき 1.0 を返します。

### ③ シャドウシェーダーのカリングは CULL_NONE

`CULL_FRONT` を使うと背面深度が書き込まれ、床との深度差が消えて影が出なくなります。

```cpp
rdc.CullMode = D3D11_CULL_NONE;
```

### ④ フラスタムは部屋サイズに合わせる

フラスタムが大きすぎると 1 テクセルあたりの範囲が広くなり影が粗くなります。

```cpp
float width  = 5.0f;  // 元 20.0f → 4倍解像度アップ
float height = 5.0f;
```

---

## 変更ファイル一覧

### `Stage.h`：CONSTANTBUFFER_STAGE に matLightVP を追加

```cpp
struct CONSTANTBUFFER_STAGE
{
    XMFLOAT4   lightPosition;
    XMFLOAT4   eyePosition;
    int        lightType;
    XMFLOAT3   _pad;
    XMFLOAT4X4 matLightVP;     // ← 追加：ライト視点の VP 行列
};
```

### `Stage.cpp Update()`：matLightVP を計算して送信

```cpp
XMMATRIX lightV  = Direct3D::GetLightViewMatrix();
XMMATRIX lightP  = Direct3D::GetLightProjectionMatrix();
XMMATRIX lightVP = lightV * lightP;
XMStoreFloat4x4(&cb.matLightVP, lightVP); // row_major 指定なので転置不要
```

### `Stage.cpp Draw()`：SRV のセットと解除

```cpp
// パス2 開始前
ID3D11ShaderResourceView* pShadowSRV = Direct3D::GetShadowMapSRV();
Direct3D::pContext->PSSetShaderResources(1, 1, &pShadowSRV);

// ... 描画 ...

// パス2 終了後（次フレームの DSV/SRV 同時バインドを防ぐ）
ID3D11ShaderResourceView* nullSRV = nullptr;
Direct3D::pContext->PSSetShaderResources(1, 1, &nullSRV);
```

### `Stage.cpp Initialize()`：比較サンプラーを作成

```cpp
D3D11_SAMPLER_DESC sampDesc = {};
sampDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.BorderColor[0] = 1.0f;  // 範囲外は「影なし」扱い
sampDesc.BorderColor[1] = 1.0f;
sampDesc.BorderColor[2] = 1.0f;
sampDesc.BorderColor[3] = 1.0f;
sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
sampDesc.MinLOD         = 0;
sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;

ID3D11SamplerState* pShadowSampler = nullptr;
Direct3D::pDevice->CreateSamplerState(&sampDesc, &pShadowSampler);
Direct3D::pContext->PSSetSamplers(1, 1, &pShadowSampler);
SAFE_RELEASE(pShadowSampler);
```

### `Simple3D.hlsl`：シャドウマップ・比較サンプラー宣言と影判定追加

```hlsl
// テクスチャ・サンプラー宣言に追記
Texture2D              g_shadowMap    : register(t1);
SamplerComparisonState g_shadowSampler: register(s1);

// cbuffer gStage に追記
row_major float4x4 matLightVP;

// PS() の return color; 直前に追記
float shadow = 1.0;
float4 lightClipPos = mul(inData.wpos, matLightVP);

float2 shadowUV;
shadowUV.x =  lightClipPos.x / lightClipPos.w * 0.5 + 0.5;
shadowUV.y = -lightClipPos.y / lightClipPos.w * 0.5 + 0.5;

if (shadowUV.x >= 0.0 && shadowUV.x <= 1.0 &&
    shadowUV.y >= 0.0 && shadowUV.y <= 1.0)
{
    float currentDepth = lightClipPos.z / lightClipPos.w;
    float bias = 0.005;
    shadow = g_shadowMap.SampleCmpLevelZero(g_shadowSampler, shadowUV, currentDepth - bias);
}

color *= (0.3 + 0.7 * shadow);  // 影の中は 30% の明るさを残す
```

### `Engine/Direct3D.cpp`：GetLightViewMatrix・シャドウシェーダー・フラスタム修正

```cpp
// GetLightViewMatrix（lightEye は +lightDir 方向）
XMVECTOR lightEye = lightDir * 10.0f;  // negation 不要

// InitShadowShader のラスタライザー
rdc.CullMode = D3D11_CULL_NONE;  // CULL_FRONT だと背面深度が書き込まれ誤判定

// GetLightProjectionMatrix のフラスタム
float width  = 5.0f;  // 部屋サイズに合わせて縮小
float height = 5.0f;
```

---

## ✅ ここでビルドして実行

1. ビルドして実行する
2. **ドーナツの影が床に落ちれば成功！**
3. WASD / Up/Down でライト方向を変えると影が動くことを確認する
4. シャドウアクネ（ドーナツ表面のノイズ）が出た場合は `bias` を `0.007` まで上げる

---

## 次のステップ

[Step6 → デバッグ UI とパラメータ調整](./Step6_Debug.md)