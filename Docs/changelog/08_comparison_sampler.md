# 変更ログ：比較サンプラーに切り替えて影のエッジをなめらかにする

## 目的
第7章の通常サンプラー + Sample() を、比較サンプラー + SampleCmpLevelZero() に差し替える。
影のエッジのジャギーが改善される。

---

## 変更ファイル

### Simple3D.hlsl

#### 宣言部の変更

```hlsl
// Before
SamplerState g_shadowSampler : register(s1);

// After
SamplerComparisonState g_shadowSampler : register(s1);
```

#### PS() 内の影判定の変更

```hlsl
// Before
float shadowDepth = g_shadowMap.Sample(g_shadowSampler, shadowUV).r;
shadow = (currentDepth - bias <= shadowDepth) ? 1.0 : 0.0;

// After
shadow = g_shadowMap.SampleCmpLevelZero(g_shadowSampler, shadowUV, currentDepth - bias);
```

### Stage.cpp

#### Initialize() への追加（比較サンプラーの作成と s1 へのセット）

```cpp
D3D11_SAMPLER_DESC sd = {};
sd.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
sd.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
sd.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
sd.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
sd.BorderColor[0] = 1.0f;  // 範囲外は影なし扱い
sd.BorderColor[1] = 1.0f;
sd.BorderColor[2] = 1.0f;
sd.BorderColor[3] = 1.0f;
sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
ID3D11SamplerState* pShadowSampler = nullptr;
Direct3D::pDevice->CreateSamplerState(&sd, &pShadowSampler);
Direct3D::pContext->PSSetSamplers(1, 1, &pShadowSampler);
SAFE_RELEASE(pShadowSampler);
```

---

## 元に戻す方法

```bash
git checkout HEAD~1 -- Simple3D.hlsl Stage.cpp
```

---

## 対応するコミット

chapter8: 比較サンプラーで影のエッジをなめらかにする
