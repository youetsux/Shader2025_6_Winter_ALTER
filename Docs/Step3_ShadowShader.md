# Step 3 ── シャドウ用シェーダーを作成する

## 学習目標

- 目的ごとにシェーダーファイルを使い分けるという考え方を覚える
- ピクセルシェーダーが何も返さなくても、深度バッファへの書き込みは GPU が自動でやってくれることを知る
- `Direct3D` に新しいシェーダーを追加する手順を復習する

---

## 理論：シャドウパスのシェーダーは何をするのか

通常の描画シェーダー（`Simple3D.hlsl`）は色を計算して画面に出力します。

```
頂点シェーダー : 頂点をスクリーン座標に変換する
ピクセルシェーダー : ライト計算をして最終的な色を返す  ← やることが多い
```

シャドウパスでは**色は一切不要**です。深度（ライトからの距離）だけが目的です。

```
頂点シェーダー : 頂点を「ライト視点の」スクリーン座標に変換するだけ
ピクセルシェーダー : 何もしない（深度は GPU が自動で書いてくれる）
```

シャドウシェーダーは `Simple3D.hlsl` よりずっとシンプルです。

---

## この Step でやること（作業順）

| 変更 | ファイル | 内容 |
|------|----------|------|
| 変更① | `ShadowMap.hlsl`（新規） | シャドウ用シェーダーを作る |
| 変更② | `Engine/Direct3D.cpp` | `InitShadowShader()` の実装を追加する |
| 変更③ | `Engine/Direct3D.cpp` | `InitShader()` から `InitShadowShader()` を呼ぶ |

> **注意：** `Engine/Direct3D.h` の `SHADER_SHADOWMAP` と `InitShadowShader()` 宣言は  
> Step2 の作業中にすでに追加済みです。変更不要です。

---

## 変更① 新規ファイル `ShadowMap.hlsl` を作る

`Simple3D.hlsl` と**同じフォルダ**（プロジェクトのルート）に新しく作成します。

```hlsl
//───────────────────────────────────────
// ShadowMap.hlsl
// シャドウマップ生成用シェーダー
// ・ライト視点でシーンを描画し、深度値をテクスチャに書き込む
// ・色の計算は不要。深度だけが目的
//───────────────────────────────────────

// コンスタントバッファ（ライト視点の WVP 行列だけ受け取る）
cbuffer cbShadow : register(b0)
{
    row_major float4x4 matLightWVP; // ライト視点の World×View×Projection
};

//───────────────────────────────────────
// 頂点シェーダー
// やること：ライト視点の WVP 行列で頂点を変換するだけ
//───────────────────────────────────────
float4 VS(float4 pos : POSITION) : SV_POSITION
{
    return mul(pos, matLightWVP);
}

//───────────────────────────────────────
// ピクセルシェーダー
// やること：何もしない
// GPU が SV_POSITION の Z 値を自動的に深度バッファに書き込んでくれる
//───────────────────────────────────────
void PS(float4 pos : SV_POSITION)
{
}
```

---

## 変更② `Engine/Direct3D.cpp` に `InitShadowShader()` の実装を追加する

`InitShadowMap()` の直後（ファイル末尾付近）に追記します。

```cpp
HRESULT Direct3D::InitShadowShader()
{
    HRESULT hr;

    // 頂点シェーダーをコンパイル
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

    // ピクセルシェーダーをコンパイル
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

    // 頂点レイアウト（POSITION だけ使う。UV や法線は不要）
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

    // ラスタライザー
    // CullMode を CULL_FRONT にするとシャドウアクネ（影のノイズ）が減る
    D3D11_RASTERIZER_DESC rdc = {};
    rdc.CullMode             = D3D11_CULL_FRONT;
    rdc.FillMode             = D3D11_FILL_SOLID;
    rdc.FrontCounterClockwise = FALSE;
    rdc.DepthClipEnable      = TRUE;
    pDevice->CreateRasterizerState(&rdc, &shaderBundle[SHADER_SHADOWMAP].pRasterizerState);

    return S_OK;
}
```

---

## 変更③ `InitShader()` から `InitShadowShader()` を呼ぶ

`InitShader()` の中、`InitToonShader()` の呼び出しの直後に1行追加します。

**変更前：**
```cpp
    if (FAILED(InitToonShader()))  { return E_FAIL; }
    return S_OK;
```

**変更後：**
```cpp
    if (FAILED(InitToonShader()))   { return E_FAIL; }
    if (FAILED(InitShadowShader())) { return E_FAIL; } // ← 追加
    return S_OK;
```

---

## ? ここでビルドして実行する

1. ビルドして実行する（画面の見た目はまだ変わりません）
2. クラッシュしなければ成功
3. エラーが出た場合は `ShadowMap.hlsl` が `Simple3D.hlsl` と同じフォルダにあるか確認する

> **ポイント：** シャドウシェーダーは「深度しか書かない」ので  
> テクスチャ・ライト計算・UV は一切不要です。  
> `Simple3D.hlsl` と比べてどれだけシンプルか確認してみましょう。

---

## よくある疑問

**Q. ラスタライザーの CullMode が `CULL_FRONT` なのはなぜ？**  
A. 通常の描画は `CULL_BACK`（裏面を描かない）ですが、  
　シャドウパスで表面のかわりに裏面を描くことで  
　「シャドウアクネ」という影のノイズを軽減できます。  
　Step6 で両者を比較してみましょう。

**Q. ピクセルシェーダーが `void` でも大丈夫？**  
A. 大丈夫です。深度バッファへの書き込みは `SV_POSITION` の Z 値を使って  
　GPU が自動でやってくれます。ピクセルシェーダーは空でも問題ありません。

---

## 次のステップ

[Step4 → 2パス描画を組み込む](./Step4_TwoPass.md)
