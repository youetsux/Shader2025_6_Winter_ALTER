# 第3章：シャドウマップ用テクスチャを作る

## この章の目的

ライトから見たZ値を保存するためのテクスチャを作る。  
まだそのテクスチャには何も描かない。

この章でも画面は変わらない。

---

## 学生向け説明

シャドウマップは普通の色画像ではない。

```text
色を保存する画像ではなく、ライトから見たZ値を保存する画像
```

DirectXでは、テクスチャ本体と、その使い道を分けて考える。

```text
ID3D11Texture2D
  ├─ DepthStencilView   : 深度を書き込む口
  └─ ShaderResourceView : シェーダーで読む口
```

同じテクスチャを、

```text
パス1では DSV として使う
パス2では SRV として使う
```

という使い方にする。

---

## 変更ファイル

- `Engine/Direct3D.h`
- `Engine/Direct3D.cpp`

---

## Copilotへの指示

```text
ステンシルは使わないシャドウマップ実装の第3章です。

Direct3Dに、シャドウマップ用の深度テクスチャを作成する処理を追加してください。

変更内容：
1. Engine/Direct3D.h に次の関数宣言を追加する。
   - HRESULT InitShadowMap(int width, int height);
   - ID3D11ShaderResourceView* GetShadowMapSRV();

2. Engine/Direct3D.cpp の namespace Direct3D 内に、次の変数を追加する。
   - int screenWidth
   - int screenHeight
   - ID3D11Texture2D* pShadowMapTexture
   - ID3D11DepthStencilView* pShadowMapDSV
   - ID3D11ShaderResourceView* pShadowMapSRV

3. Initialize(int winW, int winH, HWND hWnd) の冒頭で screenWidth と screenHeight を保存する。

4. Initialize() 内の InitShader() 後に InitShadowMap(1024, 1024) を呼ぶ。

5. Release() に pShadowMapSRV / pShadowMapDSV / pShadowMapTexture の SAFE_RELEASE を追加する。

6. InitShadowMap() を実装する。
   - Texture2D本体は DXGI_FORMAT_R32_TYPELESS
   - BindFlags は D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
   - DSV は DXGI_FORMAT_D32_FLOAT
   - SRV は DXGI_FORMAT_R32_FLOAT

7. GetShadowMapSRV() は pShadowMapSRV を返す。

既存の Direct3D の書き方に合わせ、不要な新規クラスは作らないでください。
```

---

## 実装の重要ポイント

### テクスチャ本体

```cpp
texDesc.Format    = DXGI_FORMAT_R32_TYPELESS;
texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
```

`TYPELESS` は「あとから用途を決める」という意味。

### 書き込み口 DSV

```cpp
dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
```

深度として書き込む。

### 読み込み口 SRV

```cpp
srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
```

シェーダーで浮動小数として読む。

---

## この章でのコード変更点

### 変更の概要

| ファイル | 変更内容 |
|----------|----------|
| `Engine/Direct3D.h` | 関数宣言を2つ追加 |
| `Engine/Direct3D.cpp` | 変数5つ追加・Initialize/Release修正・関数2つ追加 |

画面は変わらない。まだ `InitShadowMap()` は呼ばれているが、描画には使われていないため。

---

### `Engine/Direct3D.h` の変更

#### Before（変更前）
```cpp
DirectX::XMMATRIX GetLightViewMatrix();
DirectX::XMMATRIX GetLightProjectionMatrix();
```

#### After（変更後）
```cpp
DirectX::XMMATRIX GetLightViewMatrix();
DirectX::XMMATRIX GetLightProjectionMatrix();

HRESULT InitShadowMap(int width, int height);     // ← 追加：シャドウマップ用テクスチャ作成
ID3D11ShaderResourceView* GetShadowMapSRV();      // ← 追加：シェーダーで読む口を返す
```

---

### `Engine/Direct3D.cpp` の変更①：変数の追加

#### Before（変更前）
```cpp
namespace Direct3D
{
    // ...
    SHADER_BUNDLE shaderBundle[SHADER_MAX];
    XMFLOAT4 lightPosition{ 0.0f, 0.5f, 0.0f, 0.0f };
}
```

#### After（変更後）
```cpp
namespace Direct3D
{
    // ...
    SHADER_BUNDLE shaderBundle[SHADER_MAX];
    XMFLOAT4 lightPosition{ 0.0f, 0.5f, 0.0f, 0.0f };

    int screenWidth  = 0;  // ← 追加：画面幅（EndShadowPassでビューポートを戻すために使う）
    int screenHeight = 0;  // ← 追加：画面高さ

    ID3D11Texture2D*          pShadowMapTexture = nullptr;  // ← 追加：深度テクスチャ本体
    ID3D11DepthStencilView*   pShadowMapDSV     = nullptr;  // ← 追加：書き込み口（パス1用）
    ID3D11ShaderResourceView* pShadowMapSRV     = nullptr;  // ← 追加：読み込み口（パス2用）
}
```

---

### `Engine/Direct3D.cpp` の変更②：`Initialize()` への追加

#### Before（変更前）
```cpp
HRESULT Direct3D::Initialize(int winW, int winH, HWND hWnd)
{
    // ...
    hr = InitShader();
    if (FAILED(hr)) return hr;

    return S_OK;
}
```

#### After（変更後）
```cpp
HRESULT Direct3D::Initialize(int winW, int winH, HWND hWnd)
{
    screenWidth  = winW;  // ← 追加：画面サイズを保存
    screenHeight = winH;  // ← 追加

    // ...
    hr = InitShader();
    if (FAILED(hr)) return hr;

    hr = InitShadowMap(1024, 1024);  // ← 追加：シャドウマップ用テクスチャを1024x1024で作成
    if (FAILED(hr)) return hr;

    return S_OK;
}
```

**1024×1024 の意味：**  
シャドウマップの解像度。大きいほど影が細かくなるが、メモリを多く使う。  
授業では 1024 で十分。

---

### `Engine/Direct3D.cpp` の変更③：`Release()` への追加

#### Before（変更前）
```cpp
void Direct3D::Release()
{
    // ...
    SAFE_RELEASE(pRenderTargetView);
}
```

#### After（変更後）
```cpp
void Direct3D::Release()
{
    // ...
    SAFE_RELEASE(pShadowMapSRV);      // ← 追加
    SAFE_RELEASE(pShadowMapDSV);      // ← 追加
    SAFE_RELEASE(pShadowMapTexture);  // ← 追加
    SAFE_RELEASE(pRenderTargetView);
}
```

**解放の順番：** SRV → DSV → Texture の順。使う側から先に解放する。

---

### `Engine/Direct3D.cpp` の変更④：`InitShadowMap()` の実装（最重要）

```cpp
HRESULT Direct3D::InitShadowMap(int width, int height)
{
    HRESULT hr;

    // ========== ① テクスチャ本体を作る ==========
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width          = width;
    texDesc.Height         = height;
    texDesc.MipLevels      = 1;
    texDesc.ArraySize      = 1;
    texDesc.Format         = DXGI_FORMAT_R32_TYPELESS;  // ← あとから用途を決める
    texDesc.SampleDesc     = { 1, 0 };
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // ← 2つの口をつける
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;

    hr = pDevice->CreateTexture2D(&texDesc, nullptr, &pShadowMapTexture);
    if (FAILED(hr)) { MessageBox(nullptr, L"ShadowMap Texture の作成に失敗しました", L"エラー", MB_OK); return hr; }

    // ========== ② 書き込み口（DSV）を作る ==========
    // パス1でライト視点から深度を書き込む口
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format             = DXGI_FORMAT_D32_FLOAT;  // ← 深度として書き込む
    dsvDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = pDevice->CreateDepthStencilView(pShadowMapTexture, &dsvDesc, &pShadowMapDSV);
    if (FAILED(hr)) { MessageBox(nullptr, L"ShadowMap DSV の作成に失敗しました", L"エラー", MB_OK); return hr; }

    // ========== ③ 読み込み口（SRV）を作る ==========
    // パス2でシェーダーがサンプリングする口
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;  // ← 浮動小数として読む
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels       = 1;

    hr = pDevice->CreateShaderResourceView(pShadowMapTexture, &srvDesc, &pShadowMapSRV);
    if (FAILED(hr)) { MessageBox(nullptr, L"ShadowMap SRV の作成に失敗しました", L"エラー", MB_OK); return hr; }

    return S_OK;
}
```

**なぜ TYPELESS か：**

```text
DXGI_FORMAT_D32_FLOAT は DSV 専用で、SRV には使えない。
DXGI_FORMAT_R32_FLOAT は SRV 専用で、DSV には使えない。

→ どちらにも使えるように、テクスチャ本体を「用途未定（TYPELESS）」にしておく。
   用途は DSV と SRV それぞれを作るときに決める。
```

**BindFlags を2つ立てる意味：**
```cpp
D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
```
```text
「このテクスチャは DSV としても SRV としても使いますよ」と DirectX に伝える。
片方しか指定しないと、もう片方のビューが作れない。
```

---

### 変更のイメージ図

```text
【変更前】
通常の深度バッファしかない

  pDepthStencil     : 画面描画用の深度テクスチャ
  pDepthStencilView : 画面描画用の DSV

【変更後】
シャドウマップ専用の深度テクスチャが追加された

  pDepthStencil     : 画面描画用の深度テクスチャ（変更なし）
  pDepthStencilView : 画面描画用の DSV（変更なし）

  pShadowMapTexture : シャドウマップ用の深度テクスチャ（追加）
  pShadowMapDSV     : パス1で深度を書き込む口（追加）
  pShadowMapSRV     : パス2でシェーダーが読む口（追加）
```

---

## ビルド確認

- ビルドが通れば成功。
- 画面は変わらない。

---

## 推奨コミット

```bash
git add Engine/Direct3D.h Engine/Direct3D.cpp
git commit -m "Create shadow map depth texture and shader resource view"
```
