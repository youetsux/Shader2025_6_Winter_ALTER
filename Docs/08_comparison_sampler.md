# 第8章：比較サンプラーで影のエッジをなめらかにする

## この章でやること

影のエッジのジャギー（ギザギザ）を改善します。
第7章では Sample() で1点だけ読んで影を判定していました。
この章では SampleCmpLevelZero() に差し替えて、近隣の複数点を平均した判定にします。

この章が終わると影のエッジが少しなめらかになります。

---

## この章でのコード変更点

| ファイル | 変更内容 |
|---|---|
| Simple3D.hlsl | SamplerState を SamplerComparisonState に変更（1行） |
| Simple3D.hlsl | Sample() を SampleCmpLevelZero() に変更（2行） |
| Stage.cpp | Initialize() に比較サンプラーの作成・s1 へのセットを追加 |

---

## 1 なぜ比較サンプラーを使うとなめらかになるか

### ここでは何をするか

サンプラーの種類を「普通のサンプラー」から「比較サンプラー」に変えます。

### 違い

    Sample()             : 影 or 明るい（0か1）       → ジャギー
    SampleCmpLevelZero() : 0.0〜1.0 の間の値で返す    → なめらか

SampleCmpLevelZero() は近隣の複数点を読んで比較し、その平均を返します（PCFフィルタリング）。

---

## 2 Simple3D.hlsl の変更

### ここでは何をするか

サンプラーの型を変えて、影判定の命令を差し替えます。

Before（宣言部）

    SamplerState g_shadowSampler : register(s1);

After

    SamplerComparisonState g_shadowSampler : register(s1);

Before（PS() 内の影判定）

    float shadowDepth = g_shadowMap.Sample(g_shadowSampler, shadowUV).r;
    shadow = (currentDepth - bias <= shadowDepth) ? 1.0 : 0.0;

After

    shadow = g_shadowMap.SampleCmpLevelZero(g_shadowSampler, shadowUV, currentDepth - bias);
    // 比較まで自動でやってくれる。自分で if を書かなくてよい。

---

## 3 Stage.cpp：比較サンプラーを作成して s1 にセットする

### ここでは何をするか

Initialize() に比較サンプラーの作成と PSSetSamplers の呼び出しを追加します。
シェーダー側が SamplerComparisonState を使うようになったので、
C++ 側も対応したサンプラーを渡す必要があります。

Before（Initialize() にサンプラー作成コードがない）

After

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    sd.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
    sd.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
    sd.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
    sd.BorderColor[0] = 1.0f;  // 範囲外は影なし（明るい）扱い
    sd.BorderColor[1] = 1.0f;
    sd.BorderColor[2] = 1.0f;
    sd.BorderColor[3] = 1.0f;
    sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    ID3D11SamplerState* pShadowSampler = nullptr;
    Direct3D::pDevice->CreateSamplerState(&sd, &pShadowSampler);
    Direct3D::pContext->PSSetSamplers(1, 1, &pShadowSampler);
    SAFE_RELEASE(pShadowSampler);

BorderColor を 1.0f にしている理由：
シャドウマップの範囲外に出たときに「ライトから見える（影なし）」と判定させるためです。

---

## ビルド確認

- ビルドが通る
- 影のエッジが第7章よりなめらかになる
- ライト方向を変えても影がなめらかについてくる

---

## 推奨コミット

    git add Stage.cpp Simple3D.hlsl
    git commit -m "chapter8: 比較サンプラーで影のエッジをなめらかにする"
