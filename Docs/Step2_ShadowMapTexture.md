# Step 2 ── シャドウマップ用テクスチャを作成する

## 学習目標

- `ID3D11Texture2D` の「書く口（DSV）」と「読む口（SRV）」の違いを理解する
- `DXGI_FORMAT_R32_TYPELESS` という「用途を後から決めるフォーマット」を知る
- テクスチャを作るだけで、まだ描画には使わない（作ることに集中する）

---

## 理論：テクスチャには「書く口」と「読む口」がある

DirectX のテクスチャは**生のデータ領域**です。  
そのテクスチャをどう使うかは「ビュー（View）」によって決まります。

```
ID3D11Texture2D（テクスチャ本体）
    ├── ID3D11DepthStencilView (DSV)   ← 深度を「書き込む」ための口
    └── ID3D11ShaderResourceView (SRV) ← シェーダーで「読み込む」ための口
```

シャドウマップでは**同じテクスチャを書いてから読む**必要があるため、  
この両方のビューを同じテクスチャに対して作成します。

### なぜ `DXGI_FORMAT_R32_TYPELESS` を使うのか

| フォーマット | 用途 |
|------------|------|
| `DXGI_FORMAT_D32_FLOAT` | 深度ステンシルビュー（DSV）専用 → SRV に使えない |
| `DXGI_FORMAT_R32_FLOAT` | シェーダーリソースビュー（SRV）専用 → DSV に使えない |
| `DXGI_FORMAT_R32_TYPELESS` | **どちらにも使える**（型を後から指定） |

DSV と SRV の両方を作るために `TYPELESS` を使います。  
DSV 作成時は `DXGI_FORMAT_D32_FLOAT`、SRV 作成時は `DXGI_FORMAT_R32_FLOAT` と指定します。

---

## 変更内容

### 変更ファイル：`Engine/Direct3D.h`

シャドウマップ関連のリソースと関数を追加します。

**変更前：**
```cpp
DirectX::XMMATRIX GetLightViewMatrix();
DirectX::XMMATRIX GetLightProjectionMatrix();
```

**変更後：**
```cpp
DirectX::XMMATRIX GetLightViewMatrix();
DirectX::XMMATRIX GetLightProjectionMatrix();

// シャドウマップ用リソース
HRESULT InitShadowMap(int width, int height); // シャドウマップの初期化
ID3D11ShaderResourceView* GetShadowMapSRV();  // シェーダーで読む口を取得
```

---

### 変更ファイル：`Engine/Direct3D.cpp`

#### ① namespace 内にリソース変数を追加

`namespace Direct3D` の `{` の中、他のメンバ変数と並べて追記します。

```cpp
// ========== シャドウマップ用リソース ==========
ID3D11Texture2D*          pShadowMapTexture = nullptr; // 深度を格納するテクスチャ
ID3D11DepthStencilView*   pShadowMapDSV     = nullptr; // 書き込み口
ID3D11ShaderResourceView* pShadowMapSRV     = nullptr; // 読み込み口
// ========== シャドウマップ用リソース END ==========
```

#### ② `InitShadowMap()` 関数を追加

ファイル末尾に追記します。

```cpp
HRESULT Direct3D::InitShadowMap(int width, int height)
{
    HRESULT hr;

    // ──────────────────────────────────────────
    // ① テクスチャ本体を作成（TYPELESS = 用途を後から決める）
    // ──────────────────────────────────────────
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width          = width;                         // 解像度：幅
    texDesc.Height         = height;                        // 解像度：高さ
    texDesc.MipLevels      = 1;                             // ミップマップなし
    texDesc.ArraySize      = 1;                             // テクスチャ枚数1枚
    texDesc.Format         = DXGI_FORMAT_R32_TYPELESS;      // 型は後から決める
    texDesc.SampleDesc     = { 1, 0 };                      // アンチエイリアスなし
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL       // 深度として書く
                           | D3D11_BIND_SHADER_RESOURCE;    // シェーダーで読む
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;

    hr = pDevice->CreateTexture2D(&texDesc, nullptr, &pShadowMapTexture);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"ShadowMap Texture の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // ──────────────────────────────────────────
    // ② DSV（書き込み口）を作成：深度として書き込むための口
    // ──────────────────────────────────────────
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT; // DSV では D32_FLOAT を指定
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = pDevice->CreateDepthStencilView(pShadowMapTexture, &dsvDesc, &pShadowMapDSV);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"ShadowMap DSV の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // ──────────────────────────────────────────
    // ③ SRV（読み込み口）を作成：シェーダーからサンプリングするための口
    // ──────────────────────────────────────────
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT; // SRV では R32_FLOAT を指定
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

#### ③ `Initialize()` の中から `InitShadowMap()` を呼ぶ

`InitShader()` の呼び出しの直後に追記します。

```cpp
hr = InitShader();
if (FAILED(hr)) { return hr; }

// ========== シャドウマップ初期化 ==========
hr = InitShadowMap(1024, 1024); // 解像度 1024x1024
if (FAILED(hr)) { return hr; }
// ==========================================
```

---

### 変更ファイル：`Stage.cpp`

Step1 で追加した ImGui デバッグ表示に、テクスチャの情報を追記します。

```cpp
// ========== Step2 デバッグ：シャドウマップの情報を表示 ==========
ImGui::Text("ShadowMap SRV: %s",
    Direct3D::GetShadowMapSRV() != nullptr ? "OK" : "null");
// ================================================================
```

---

## ✅ ここでビルドして実行

1. ビルドして実行する
2. ImGui に **「ShadowMap SRV: OK」** と表示されれば成功
3. `null` が出た場合は Visual Studio の出力ウィンドウでエラーメッセージを確認する

> **ポイント：** テクスチャを作っただけでまだ何も描いていないため、  
> 画面の見た目は変わりません。「リソースの準備」と「使う」は別の作業です。

---

## よくある疑問

**Q. シャドウマップの解像度（1024×1024）はどう影響する？**  
A. 解像度が高いほど影の輪郭がきれいになりますが、メモリと描画負荷が増えます。  
　512×512 は粗め、2048×2048 は高品質です。Step6 で比較してみましょう。

**Q. `BindFlags` に2つ指定しているのはなぜ？**  
A. `|`（ビットOR）で複数の用途を同時に登録できます。  
　「深度バッファとしても使えて、シェーダーからも読める」という意味です。

---

## 次のステップ

[Step3 → シャドウ用シェーダーを作成する](./Step3_ShadowShader.md)
