# 第4章：深度だけを書く ShadowMap.hlsl を作る

## この章の目的

ライト視点で深度だけを書くための専用シェーダーを作る。  
通常描画用の `Simple3D.hlsl` とは別に、`ShadowMap.hlsl` を追加する。

この章でも画面は変わらない。

---

## 学生向け説明

通常描画では、色・テクスチャ・ライト計算を使う。

しかしシャドウマップ作成では、必要なのはライトから見たZ値だけ。

```text
必要：頂点をライト視点に変換する
不要：色、テクスチャ、ライティング
```

ピクセルシェーダーは何もしなくてよい。  
`SV_POSITION` のZ値が、自動的に深度バッファへ書き込まれる。

---

## 変更ファイル

- `ShadowMap.hlsl` 新規追加
- `Engine/Direct3D.h`
- `Engine/Direct3D.cpp`
- `MyFirstGame.vcxproj` 必要に応じて追加

---

## Copilotへの指示

```text
ステンシルは使わないシャドウマップ実装の第4章です。

シャドウマップ作成用の専用HLSLと、その初期化処理を追加してください。

変更内容：
1. プロジェクトルートに ShadowMap.hlsl を新規作成する。
   - cbuffer cbShadow : register(b0) に row_major float4x4 matLightWVP を持たせる。
   - VS は POSITION を受け取り、mul(pos, matLightWVP) を SV_POSITION として返す。
   - PS は void でよい。色は出さない。

2. Engine/Direct3D.h の SHADER_TYPE に SHADER_SHADOWMAP を追加する。
   - SHADER_MAX の前に追加する。

3. Engine/Direct3D.h に次の関数宣言を追加する。
   - HRESULT InitShadowShader();
   - void BeginShadowPass();
   - void EndShadowPass();

4. Engine/Direct3D.cpp の InitShader() から InitShadowShader() を呼ぶ。

5. InitShadowShader() を実装する。
   - ShadowMap.hlsl の VS / PS をコンパイルする。
   - InputLayout は POSITION のみ。
   - ラスタライザーは D3D11_CULL_NONE にする。

6. BeginShadowPass() を実装する。
   - pShadowMapDSV を ClearDepthStencilView で 1.0f にクリアする。
   - OMSetRenderTargets で RTV を nullptr、DSV を pShadowMapDSV にする。
   - Viewport をシャドウマップサイズにする。
   - SetShader(SHADER_SHADOWMAP) を呼ぶ。

7. EndShadowPass() を実装する。
   - OMSetRenderTargets を通常の pRenderTargetView / pDepthStencilView に戻す。
   - Viewport を screenWidth / screenHeight に戻す。

ステンシル処理は追加しないでください。
```

---

## `ShadowMap.hlsl` の内容

```hlsl
cbuffer cbShadow : register(b0)
{
    row_major float4x4 matLightWVP;
};

float4 VS(float4 pos : POSITION) : SV_POSITION
{
    return mul(pos, matLightWVP);
}

void PS(float4 pos : SV_POSITION)
{
    // 何もしない。
    // GPU が SV_POSITION の Z 値を深度バッファに書き込む。
}
```

---

## 注意点

### InputLayoutはPOSITIONだけ

シャドウマップではUVや法線は使わない。

```cpp
D3D11_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
```

### 色を書かない

`BeginShadowPass()` ではレンダーターゲットビューを `nullptr` にする。

```cpp
ID3D11RenderTargetView* nullRTV = nullptr;
pContext->OMSetRenderTargets(1, &nullRTV, pShadowMapDSV);
```

---

## ビルド確認

- ビルドが通れば成功。
- 画面は変わらない。

---

## 推奨コミット

```bash
git add ShadowMap.hlsl Engine/Direct3D.h Engine/Direct3D.cpp MyFirstGame.vcxproj
git commit -m "Add shadow map depth-only shader pass"
```
