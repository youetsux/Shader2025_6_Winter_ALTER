# Step2 ── シャドウマップ用テクスチャを作る

## 何をするか

シャドウマップは「ライト視点から見た深度値を書き込んだテクスチャ」のこと。  
この Step では、そのテクスチャを作る。まだ何も書き込まない。

画面は変わらない。

---

## 知識：テクスチャには「書く口」と「読む口」がある

DirectX のテクスチャは生のデータ領域で、使い方はビューで決まる。

```
ID3D11Texture2D（テクスチャ本体）
    ├── DepthStencilView  (DSV) ── 深度を書き込む口
    └── ShaderResourceView(SRV) ── シェーダーで読む口
```

シャドウマップでは同じテクスチャに対してこの両方を作る。  
パス1（シャドウパス）で DSV に深度を書き込み、  
パス2（メインパス）で SRV をシェーダーから読む。

---

## 知識：DXGI_FORMAT_R32_TYPELESS を使う理由

| フォーマット | 使える口 |
|---|---|
| DXGI_FORMAT_D32_FLOAT | DSV のみ（SRV に使えない）|
| DXGI_FORMAT_R32_FLOAT | SRV のみ（DSV に使えない）|
| DXGI_FORMAT_R32_TYPELESS | **両方使える（後で用途を決める）** |

テクスチャ本体は TYPELESS で作り、DSV では D32_FLOAT、SRV では R32_FLOAT として使う。

---

## 実装

### Engine/Direct3D.h

`namespace Direct3D` の変数宣言に追加する。  
（`pDevice` や `pContext` が `extern` されている周辺）

```cpp
// 既存
extern ID3D11Device* pDevice;
extern ID3D11DeviceContext* pContext;

// 追加：SHADER_SHADOWMAP の enum 値も追加が必要（Step3 で使う）
```

関数宣言に追加する。

```cpp
HRESULT InitShadowMap(int width, int height); // 追加
ID3D11ShaderResourceView* GetShadowMapSRV();  // 追加
```

### Engine/Direct3D.cpp

`namespace Direct3D { }` の中の変数定義に追加する。  
（`XMFLOAT4 lightPosition` が書いてある周辺）

```cpp
// 追加
int screenWidth  = 0;
int screenHeight = 0;

ID3D11Texture2D*          pShadowMapTexture = nullptr;
ID3D11DepthStencilView*   pShadowMapDSV     = nullptr;
ID3D11ShaderResourceView* pShadowMapSRV     = nullptr;
```

`Initialize()` の中で、`InitShader()` の後に追加する。

```cpp
hr = InitShader();
if (FAILED(hr)) { return hr; }

// 追加
hr = InitShadowMap(1024, 1024);
if (FAILED(hr)) { return hr; }
```

`Initialize()` の冒頭で画面サイズを保存する。  
（後で EndShadowPass のビューポートを戻すために使う）

```cpp
HRESULT Direct3D::Initialize(int winW, int winH, HWND hWnd)
{
    screenWidth  = winW; // 追加
    screenHeight = winH; // 追加
    // 以下既存のコード...
```

`Release()` にシャドウマップの解放を追加する。  
（他の SAFE_RELEASE と同じ場所）

```cpp
SAFE_RELEASE(pShadowMapSRV);
SAFE_RELEASE(pShadowMapDSV);
SAFE_RELEASE(pShadowMapTexture);
```

`InitShadowMap()` と `GetShadowMapSRV()` の実装を追加する。  
（`SetLightPos` の定義の下あたり）

```cpp
HRESULT Direct3D::InitShadowMap(int width, int height)
{
    HRESULT hr;

    // テクスチャ本体：TYPELESS で作ることで DSV / SRV 両方に使える
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width          = width;
    texDesc.Height         = height;
    texDesc.MipLevels      = 1;
    texDesc.ArraySize      = 1;
    texDesc.Format         = DXGI_FORMAT_R32_TYPELESS;
    texDesc.SampleDesc     = { 1, 0 };
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;

    hr = pDevice->CreateTexture2D(&texDesc, nullptr, &pShadowMapTexture);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"ShadowMap Texture の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // DSV：深度を書き込む口
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format             = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = pDevice->CreateDepthStencilView(pShadowMapTexture, &dsvDesc, &pShadowMapDSV);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"ShadowMap DSV の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // SRV：シェーダーで読む口
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels       = 1;

    hr = pDevice->CreateShaderResourceView(pShadowMapTexture, &srvDesc, &pShadowMapSRV);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"ShadowMap SRV の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    return S_OK;
}

ID3D11ShaderResourceView* Direct3D::GetShadowMapSRV()
{
    return pShadowMapSRV;
}
```

---

## ビルドして確認

ビルドが通れば OK。画面は変わらない。

次 → [Step3](./Step3.md)
