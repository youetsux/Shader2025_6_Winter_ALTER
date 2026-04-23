# Step 3 ── シャドウ用シェーダーを作成する

## 学習目標

- 「目的ごとにシェーダーを使い分ける」という発想を身につける
- ピクセルシェーダーが何も返さなくても深度バッファへの書き込みは自動で行われることを知る
- `Direct3D` に新しいシェーダーを登録する手順を復習する

---

## 理論：シャドウパスのシェーダーは何をするのか

通常の描画シェーダー（`Simple3D.hlsl`）は色を計算して画面に出力します。

```
VS（頂点シェーダー）: 頂点をスクリーン座標に変換
PS（ピクセルシェーダー）: ライト計算をして最終的な色を返す  ← 複雑
```

シャドウパスでは**色は不要**です。深度だけが目的です。

```
VS（頂点シェーダー）: 頂点を「ライト視点の」スクリーン座標に変換
PS（ピクセルシェーダー）: 何もしない（深度は GPU が自動で書いてくれる）
```

つまりシャドウシェーダーは**非常にシンプル**です。  
ピクセルシェーダーで何も返さなくても、深度バッファへの書き込みは GPU が自動で行います。

---

## 変更内容

### 新規ファイル：`ShadowMap.hlsl`

プロジェクトのルート（`Simple3D.hlsl` と同じ場所）に新規作成します。

```hlsl
//───────────────────────────────────────
// ShadowMap.hlsl
// シャドウマップ生成用シェーダー
// ・ライト視点でシーンを描画し、深度値をテクスチャに書き込む
// ・色の計算は不要。深度だけが目的
//───────────────────────────────────────

// コンスタントバッファ（ライト視点のWVP行列のみ）
cbuffer cbShadow : register(b0)
{
    row_major float4x4 matLightWVP; // ライト視点のWorld×View×Projection
};

//───────────────────────────────────────
// 頂点シェーダー
// ・やること：ライト視点の WVP 行列で頂点をクリップ空間に変換するだけ
//───────────────────────────────────────
float4 VS(float4 pos : POSITION) : SV_POSITION
{
    return mul(pos, matLightWVP);
}

//───────────────────────────────────────
// ピクセルシェーダー
// ・やること：何もしない
// ・深度バッファへの書き込みは GPU が自動で行う
//───────────────────────────────────────
void PS(float4 pos : SV_POSITION)
{
    // 何も書かなくてよい
    // GPU は SV_POSITION の Z 値を自動的に深度バッファに書き込む
}
```

---

### 変更ファイル：`Engine/Direct3D.h`

`SHADER_TYPE` 列挙型に `SHADER_SHADOWMAP` を追加します。

**変更前：**
```cpp
enum SHADER_TYPE
{
    SHADER_3D,
    SHADER_2D,
    SHADER_NORMALMAP,
    SHADER_TOON,
    SHADER_MAX
};
```

**変更後：**
```cpp
enum SHADER_TYPE
{
    SHADER_3D,
    SHADER_2D,
    SHADER_NORMALMAP,
    SHADER_TOON,
    SHADER_SHADOWMAP, // ← シャドウマップ生成用
    SHADER_MAX
};
```

また関数宣言も追加します。

```cpp
HRESULT InitShadowShader(); // シャドウマップ用シェーダー初期化
```

---

### 変更ファイル：`Engine/Direct3D.cpp`

#### ① `InitShadowShader()` 関数を追加

```cpp
HRESULT Direct3D::InitShadowShader()
{
    HRESULT hr;

    // 頂点シェーダーのコンパイル
    ID3DBlob* pCompileVS = nullptr;
    D3DCompileFromFile(L"ShadowMap.hlsl", nullptr, nullptr,
        "VS", "vs_5_0", 0, 0, &pCompileVS, nullptr);
    assert(pCompileVS != nullptr);

    hr = pDevice->CreateVertexShader(
        pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(),
        nullptr, &shaderBundle[SHADER_SHADOWMAP].pVertexShader);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Shadow 頂点シェーダーの作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // ピクセルシェーダーのコンパイル
    ID3DBlob* pCompilePS = nullptr;
    D3DCompileFromFile(L"ShadowMap.hlsl", nullptr, nullptr,
        "PS", "ps_5_0", 0, 0, &pCompilePS, nullptr);
    assert(pCompilePS != nullptr);

    hr = pDevice->CreatePixelShader(
        pCompilePS->GetBufferPointer(), pCompilePS->GetBufferSize(),
        nullptr, &shaderBundle[SHADER_SHADOWMAP].pPixelShader);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Shadow ピクセルシェーダーの作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // 頂点インプットレイアウト（POSITION だけ使う）
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = pDevice->CreateInputLayout(layout, 1,
        pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(),
        &shaderBundle[SHADER_SHADOWMAP].pVertexLayout);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Shadow InputLayout の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    pCompileVS->Release();
    pCompilePS->Release();

    // ラスタライザー（背面カリングを逆にするとシャドウアクネが減ることがある）
    D3D11_RASTERIZER_DESC rdc = {};
    rdc.CullMode = D3D11_CULL_FRONT; // シャドウパスは前面カリング（裏面を描く）
    rdc.FillMode = D3D11_FILL_SOLID;
    rdc.FrontCounterClockwise = FALSE;
    rdc.DepthClipEnable = TRUE;
    pDevice->CreateRasterizerState(&rdc, &shaderBundle[SHADER_SHADOWMAP].pRasterizerState);

    return S_OK;
}
```

#### ② `InitShader()` から `InitShadowShader()` を呼ぶ

```cpp
HRESULT Direct3D::InitShader()
{
    if (FAILED(InitShader3D()))    { return E_FAIL; }
    if (FAILED(InitShader2D()))    { return E_FAIL; }
    if (FAILED(InitNormalShader())) { return E_FAIL; }
    if (FAILED(InitToonShader()))  { return E_FAIL; }
    if (FAILED(InitShadowShader())) { return E_FAIL; } // ← 追加
    return S_OK;
}
```

---

## 確認方法

1. ビルドが通れば成功（まだ画面の見た目は変わりません）
2. `ShadowMap.hlsl` のコンパイルエラーが出た場合は、  
   `D3DCompileFromFile` の第1引数のパス（`L"ShadowMap.hlsl"`）が  
   実行ファイルと同じフォルダにあるか確認してください

> **ポイント：** シャドウシェーダーは「深度しか書かない」ので  
> テクスチャ、ライト計算、UV座標などは一切不要です。  
> シンプルさがこのシェーダーの特徴です。

---

## よくある疑問

**Q. ラスタライザーの CullMode が `CULL_FRONT` なのはなぜ？**  
A. 通常の描画は `CULL_BACK`（裏面を描かない）ですが、  
　シャドウパスで裏面（内側のポリゴン）を描くことで「シャドウアクネ」という  
　細かいノイズ状の影の縞を軽減できます。Step6 で比較してみましょう。

**Q. ピクセルシェーダーが `void` でも大丈夫？**  
A. はい。`SV_POSITION` の Z 成分（深度値）は GPU が自動で深度バッファに書き込みます。  
　ピクセルシェーダーが `void` であっても深度の書き込みは行われます。

---

## 次のステップ

[Step4 → 2パス描画を組み込む](./Step4_TwoPass.md)
