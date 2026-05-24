# 第7章：Simple3D.hlslで深度比較して影を出す

## この章でやること

Simple3D.hlsl のピクセルシェーダーで、シャドウマップを読んで影を判定します。

この章で初めて画面に影が出ます。
ただし影のエッジはジャギー（ギザギザ）になります。
なめらかにするのは第8章で行います。

---

## この章でのコード変更点

| ファイル | 変更内容 |
|---|---|
| Stage.cpp | メインパスの前後にシャドウマップSRVをセット／解除 |
| Simple3D.hlsl | g_shadowMap・g_shadowSampler の宣言を追加 |
| Simple3D.hlsl | cbuffer gStage に matLightVP を追加 |
| Simple3D.hlsl | PS() に影判定コードを追加 |

---

## 1 Stage.cpp：シャドウマップSRVをセットする

### ここでは何をするか

メインパスの描画前に、第3章で作ったシャドウマップのテクスチャをピクセルシェーダーの t1 に渡します。
描画後は必ず解除します。

### なぜ解除が必要か

解除しないと次のフレームで同じテクスチャを書き込み用（DSV）と読み込み用（SRV）の両方にバインドしようとして
DirectX が警告を出し、正常に動作しなくなります。

Before（Stage.cpp の Draw() メインパス部分）

    Model::Draw(hball_);
    Model::Draw(hRoom_);
    Model::Draw(hDonut_);

After

    // シャドウマップを t1 にセット
    ID3D11ShaderResourceView* pShadowSRV = Direct3D::GetShadowMapSRV();
    Direct3D::pContext->PSSetShaderResources(1, 1, &pShadowSRV);

    Model::Draw(hball_);
    Model::Draw(hRoom_);
    Model::Draw(hDonut_);

    // 描画後は必ず解除する
    ID3D11ShaderResourceView* nullSRV = nullptr;
    Direct3D::pContext->PSSetShaderResources(1, 1, &nullSRV);

---

## 2 Simple3D.hlsl：宣言を追加する

### ここでは何をするか

シャドウマップを読むためのテクスチャ変数とサンプラーを宣言します。
あわせて cbuffer gStage に matLightVP を追加します。
C++ 側は第6章ですでに送っています。シェーダー側の受け口を追加するだけです。

Before（宣言部）

    Texture2D    g_texture : register(t0);
    SamplerState g_sampler : register(s0);

After

    Texture2D    g_texture       : register(t0);
    SamplerState g_sampler       : register(s0);
    Texture2D    g_shadowMap     : register(t1);
    SamplerState g_shadowSampler : register(s1);

Before（cbuffer gStage）

    cbuffer gStage : register(b1)
    {
        float4 lightPosition;
        float4 eyePosition;
        int    lightType;
        float3 _pad;
    };

After

    cbuffer gStage : register(b1)
    {
        float4             lightPosition;
        float4             eyePosition;
        int                lightType;
        float3             _pad;
        row_major float4x4 matLightVP;
    };

---

## 3 Simple3D.hlsl：PS() に影判定を追加する

### ここでは何をするか

今描いているピクセルが「ライトから見えているか（明るい）」
「見えていないか（影）」を判定して、色に掛けます。

### 判定の手順

1. ワールド座標をライト視点のクリップ座標に変換する
2. クリップ座標をシャドウマップのUV座標に変換する
3. シャドウマップから「ライトに一番近いZ値」を読む
4. 今のピクセルのZ値と比べる
   今のピクセルの方が奥 → 手前に何かある → 影

After（PS() の return color の直前に追加）

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

        float shadowDepth = g_shadowMap.Sample(g_shadowSampler, shadowUV).r;
        shadow = (currentDepth - bias <= shadowDepth) ? 1.0 : 0.0;
    }

    color *= (0.3 + 0.7 * shadow);

---

## bias（バイアス）とは

シャドウマップのZ値には浮動小数点の誤差があります。
バイアスなしだと、自分自身の表面が「自分より奥にある」と誤判定されて
ノイズのような影が全面に出ます（セルフシャドウ）。
bias = 0.005 は「少しだけ手前のものは影にしない」という調整値です。

---

## ビルド確認

- ビルドが通る
- ドーナツが床や部屋に影を落とす
- WASD / 上下キーでライト方向を変えると影も動く
- 影のエッジがジャギー（ギザギザ）になる → 第8章で改善する

---

## 推奨コミット

    git add Stage.cpp Simple3D.hlsl
    git commit -m "chapter7: ピクセルシェーダーで影判定を追加"