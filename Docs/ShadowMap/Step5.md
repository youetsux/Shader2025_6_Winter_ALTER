# Step5 ── シェーダーで影を判定する

## 何をするか

ここまででシャドウマップ（ライト視点の深度テクスチャ）は完成している。  
あとは Simple3D.hlsl の PS() でそれを読んで、影かどうかを判定するだけ。

この Step ではじめて影が見える。

---

## 知識：影の判定の考え方

パス1でシャドウマップに「ライトから見て最初に当たるポリゴンの深度」が書き込まれた。  
パス2のピクセルシェーダーでは以下の順で判定する。

```
① このピクセルのワールド座標をライト視点のクリップ座標に変換する
② クリップ座標 → シャドウマップの UV 座標に変換する
③ シャドウマップをサンプリング（ライトから見た深度値を取得）
④ 現在のピクセルの深度と比較する

(現在の深度) ≒ (シャドウマップの深度) → ライトから見えていた = 明るい
(現在の深度) >> (シャドウマップの深度) → 何かに遮られていた = 影
```

### クリップ座標 → UV 変換

DirectX のクリップ座標は X・Y が -1〜+1、UV は 0〜1。  
Y 軸はテクスチャ座標と向きが逆なのでマイナスをかける。

```
u =  clipPos.x / clipPos.w * 0.5 + 0.5
v = -clipPos.y / clipPos.w * 0.5 + 0.5
```

### シャドウバイアス

浮動小数点の誤差でポリゴンが自分自身に影を落とすことがある（シャドウアクネ）。  
深度値から少しだけ引くことで「自分自身は影の外」と判定させる。

```
比較値 = 現在の深度 - bias（例：0.005）
```

---

## 実装

### Stage.cpp の Draw() に SRV のセットと解除を追加する

パス2の先頭（`Model::Draw(hball_)` の前）に追加する。

```cpp
// シャドウマップ SRV をピクセルシェーダーのスロット t1 にセット
ID3D11ShaderResourceView* pShadowSRV = Direct3D::GetShadowMapSRV();
Direct3D::pContext->PSSetShaderResources(1, 1, &pShadowSRV);
```

パス2の末尾（ImGui の前）に追加する。

```cpp
// SRV を解除する（次フレームのパス1で DSV と同時バインドになるのを防ぐ）
ID3D11ShaderResourceView* nullSRV = nullptr;
Direct3D::pContext->PSSetShaderResources(1, 1, &nullSRV);
```

### Simple3D.hlsl に宣言を追加する

ファイル上部のテクスチャ・サンプラー宣言の下に追加する。

```hlsl
Texture2D              g_texture   : register(t0); // 既存
SamplerState           g_sampler   : register(s0); // 既存

Texture2D              g_shadowMap     : register(t1); // 追加
SamplerComparisonState g_shadowSampler : register(s1); // 追加
```

`cbuffer gStage` に matLightVP を追加する。

```hlsl
cbuffer gStage : register(b1)
{
    float4 lightPosition;
    float4 eyePosition;
    int    lightType;
    float3 _pad;
    row_major float4x4 matLightVP; // 追加
};
```

### Simple3D.hlsl の PS() に影判定を追加する

`return color;` の直前に追加する。

```hlsl
// ── 影の判定 ───────────────────────────────────────────────
float shadow = 1.0; // デフォルト：影なし（明るい）

// ① ワールド座標をライト視点のクリップ座標に変換
float4 lightClipPos = mul(inData.wpos, matLightVP);

// ② クリップ座標 → UV 座標に変換
float2 shadowUV;
shadowUV.x =  lightClipPos.x / lightClipPos.w * 0.5 + 0.5;
shadowUV.y = -lightClipPos.y / lightClipPos.w * 0.5 + 0.5;

// ③ UV が 0〜1 の範囲内にある場合だけ判定（範囲外はライトの視野外 = 影なし）
if (shadowUV.x >= 0.0 && shadowUV.x <= 1.0 &&
    shadowUV.y >= 0.0 && shadowUV.y <= 1.0)
{
    // ④ 現在のピクセルの深度
    float currentDepth = lightClipPos.z / lightClipPos.w;

    // バイアス：自己シャドウ（シャドウアクネ）を防ぐために少し引く
    float bias = 0.005;

    // SampleCmpLevelZero：(currentDepth - bias) <= シャドウマップの深度 なら 1.0 を返す
    shadow = g_shadowMap.SampleCmpLevelZero(
        g_shadowSampler, shadowUV, currentDepth - bias);
}

// 影の中は 30% の明るさを残す（真っ暗にならないように）
color *= (0.3 + 0.7 * shadow);
// ── 影の判定 END ────────────────────────────────────────────

return color;
```

---

## ビルドして実行する

影が出れば成功。  
WASD / Up / Down でライト方向を動かすと影も動くことを確認する。

### 影が出ない場合のチェックリスト

| 症状 | 確認場所 |
|------|---------|
| 全面影になる | hRoom_ を DrawShadow で描画していないか確認 |
| 影がまったく出ない | Step4-D の matLightVP の送信が Update() に入っているか確認 |
| ドーナツ表面にノイズ | bias を 0.007 まで上げる |
