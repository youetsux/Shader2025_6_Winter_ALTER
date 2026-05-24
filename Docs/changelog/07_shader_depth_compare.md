# 変更ログ：Simple3D.hlsl に影判定を追加（通常サンプラー版）

## 目的
ピクセルシェーダーでシャドウマップを読んで影を判定する。
この時点では通常の Sample() を使うため、影のエッジはジャギーになる。

---

## 変更ファイル

### Stage.cpp

#### Draw() の変更（メインパス部分）

```cpp
// Before
Model::Draw(hball_);
Model::Draw(hRoom_);
Model::Draw(hDonut_);

// After
ID3D11ShaderResourceView* pShadowSRV = Direct3D::GetShadowMapSRV();
Direct3D::pContext->PSSetShaderResources(1, 1, &pShadowSRV);

Model::Draw(hball_);
Model::Draw(hRoom_);
Model::Draw(hDonut_);

ID3D11ShaderResourceView* nullSRV = nullptr;
Direct3D::pContext->PSSetShaderResources(1, 1, &nullSRV);
```

### Simple3D.hlsl

#### 宣言部の変更

```hlsl
// Before
Texture2D    g_texture : register(t0);
SamplerState g_sampler : register(s0);

// After
Texture2D    g_texture       : register(t0);
SamplerState g_sampler       : register(s0);
Texture2D    g_shadowMap     : register(t1);
SamplerState g_shadowSampler : register(s1);
```

#### cbuffer gStage の変更

```hlsl
// Before
cbuffer gStage : register(b1)
{
	float4 lightPosition;
	float4 eyePosition;
	int    lightType;
	float3 _pad;
};

// After
cbuffer gStage : register(b1)
{
	float4             lightPosition;
	float4             eyePosition;
	int                lightType;
	float3             _pad;
	row_major float4x4 matLightVP;
};
```

#### PS() への追加（return color の直前）

```hlsl
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
```

---

## 元に戻す方法

```bash
git checkout HEAD~1 -- Simple3D.hlsl Stage.cpp
```

---

## 対応するコミット

chapter7: Simple3D.hlsl に影判定を追加（通常サンプラー版）
