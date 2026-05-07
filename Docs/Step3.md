# Step3 ── 深度を書くだけのシェーダーを作る

## 何をするか

シャドウパス（ライト視点での描画）専用のシェーダーを作る。  
このシェーダーは「色を出力しない、深度だけ書く」ものなので、  
Simple3D.hlsl より圧倒的にシンプルになる。

画面は変わらない。

---

## 知識：なぜ専用シェーダーが必要か

シャドウパスでは「ライト視点からポリゴンがどの深度にあるか」だけが欲しい。  
色・テクスチャ・ライティングは一切不要。

やることは1つ：
```
頂点座標 × ライトWVP行列 → SV_POSITION に出力
```

GPU は SV_POSITION の Z 値を自動で深度バッファに書いてくれる。  
ピクセルシェーダーは空っぽ（void）でよい。

---

## 実装

### ShadowMap.hlsl を新規作成

プロジェクトルート（`Simple3D.hlsl` と同じフォルダ）に作る。

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
    // 何もしない。GPU が自動で深度バッファに書いてくれる
}
```

### Engine/Direct3D.h に enum 値と関数宣言を追加

```cpp
enum SHADER_TYPE
{
    SHADER_3D,
    SHADER_2D,
    SHADER_NORMALMAP,
    SHADER_TOON,
    SHADER_SHADOWMAP, // 追加
    SHADER_MAX
};
```

```cpp
HRESULT InitShadowShader(); // 追加

void BeginShadowPass();     // 追加：シャドウ用レンダーターゲットに切り替える
void EndShadowPass();       // 追加：通常の画面に戻す
```

### Engine/Direct3D.cpp に実装を追加

`InitShader()` の末尾に呼び出しを追加する。

```cpp
if (FAILED(InitToonShader()))   { return E_FAIL; }
if (FAILED(InitShadowShader())) { return E_FAIL; } // 追加
return S_OK;
```

`InitShadowShader()` の実装を追加する。  
InputLayout は POSITION だけ（UV も法線も不要）。

```cpp
HRESULT Direct3D::InitShadowShader()
{
    HRESULT hr;

    ID3DBlob* pCompileVS = nullptr;
    D3DCompileFromFile(L"ShadowMap.hlsl", nullptr, nullptr,
        "VS", "vs_5_0", 0, 0, &pCompileVS, nullptr);
    assert(pCompileVS != nullptr);

    hr = pDevice->CreateVertexShader(
        pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(),
        nullptr, &shaderBundle[SHADER_SHADOWMAP].pVertexShader);
    if (FAILED(hr)) { return hr; }

    ID3DBlob* pCompilePS = nullptr;
    D3DCompileFromFile(L"ShadowMap.hlsl", nullptr, nullptr,
        "PS", "ps_5_0", 0, 0, &pCompilePS, nullptr);
    assert(pCompilePS != nullptr);

    hr = pDevice->CreatePixelShader(
        pCompilePS->GetBufferPointer(), pCompilePS->GetBufferSize(),
        nullptr, &shaderBundle[SHADER_SHADOWMAP].pPixelShader);
    if (FAILED(hr)) { return hr; }

    // InputLayout は POSITION だけ
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = pDevice->CreateInputLayout(layout, 1,
        pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(),
        &shaderBundle[SHADER_SHADOWMAP].pVertexLayout);
    if (FAILED(hr)) { return hr; }

    pCompileVS->Release();
    pCompilePS->Release();

    // ラスタライザー：CULL_NONE にする
    // （CULL_FRONT にすると床との深度差が消えて影が出なくなる）
    D3D11_RASTERIZER_DESC rdc = {};
    rdc.CullMode              = D3D11_CULL_NONE;
    rdc.FillMode              = D3D11_FILL_SOLID;
    rdc.FrontCounterClockwise = FALSE;
    rdc.DepthClipEnable       = TRUE;
    pDevice->CreateRasterizerState(&rdc, &shaderBundle[SHADER_SHADOWMAP].pRasterizerState);

    return S_OK;
}
```

`BeginShadowPass()` と `EndShadowPass()` の実装を追加する。

```cpp
void Direct3D::BeginShadowPass()
{
    // シャドウマップをクリア（1.0 = 最大深度）
    pContext->ClearDepthStencilView(pShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // レンダーターゲットをシャドウマップに切り替える
    // RTV = nullptr にすることで色は一切書かない
    ID3D11RenderTargetView* nullRTV = nullptr;
    pContext->OMSetRenderTargets(1, &nullRTV, pShadowMapDSV);

    // ビューポートをシャドウマップのサイズに合わせる
    D3D11_TEXTURE2D_DESC texDesc;
    pShadowMapTexture->GetDesc(&texDesc);

    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)texDesc.Width;
    vp.Height   = (float)texDesc.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    pContext->RSSetViewports(1, &vp);

    SetShader(SHADER_SHADOWMAP);
}

void Direct3D::EndShadowPass()
{
    // レンダーターゲットを通常の画面に戻す
    pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);

    // ビューポートも画面サイズに戻す
    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)screenWidth;
    vp.Height   = (float)screenHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    pContext->RSSetViewports(1, &vp);
}
```

---

## ビルドして確認

ビルドが通れば OK。画面は変わらない。

次 → [Step4](./Step4.md)
