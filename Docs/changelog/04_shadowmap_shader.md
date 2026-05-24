# 変更ログ：ShadowMap.hlsl 新規作成とシャドウパス切り替え関数を追加

## 目的
深度だけを書くための専用シェーダーと、パス切り替えのための関数を追加する。

---

## 変更ファイル

### ShadowMap.hlsl（新規作成）

```hlsl
cbuffer cbShadow : register(b0)
{
	row_major float4x4 matLightWVP;
};

struct VS_OUT
{
	float4 pos : SV_POSITION;
};

VS_OUT VS(float4 pos : POSITION)
{
	VS_OUT o;
	o.pos = mul(pos, matLightWVP);
	return o;
}

void PS() {}
```

### Engine/Direct3D.h

#### SHADER_TYPE に追加
```cpp
SHADER_SHADOWMAP,  // SHADER_MAX の前に追加
```

#### 追加した宣言
```cpp
HRESULT InitShadowShader();
void BeginShadowPass();
void EndShadowPass();
```

### Engine/Direct3D.cpp

#### InitShader() の変更
- `InitShadowShader()` を末尾で呼ぶ

#### InitShadowShader() の実装内容
- `ShadowMap.hlsl` の VS / PS をコンパイル
- InputLayout は `POSITION` のみ
- ラスタライザーは `D3D11_CULL_NONE`

#### BeginShadowPass() の実装内容
- `pShadowMapDSV` を `1.0f` でクリア
- `OMSetRenderTargets(0, nullptr, pShadowMapDSV)`
- Viewport をシャドウマップサイズ（1024×1024）に設定
- `SetShader(SHADER_SHADOWMAP)` を呼ぶ

#### EndShadowPass() の実装内容
- `OMSetRenderTargets` を通常の RTV / DSV に戻す
- Viewport を `screenWidth` / `screenHeight` に戻す

---

## 元に戻す方法

```bash
git checkout HEAD~1 -- ShadowMap.hlsl Engine/Direct3D.h Engine/Direct3D.cpp
```

---

## 対応するコミット

chapter4: ShadowMap.hlsl とシャドウパス切り替え関数を追加
