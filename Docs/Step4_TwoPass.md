# Step 4 ── 2パス描画を組み込む

## 学習目標

- 「同じモデルを2回描く」という2パス描画の構造を理解する
- レンダーターゲットの切り替え（シャドウ用 ↔ 通常）の方法を知る
- `Fbx::Draw()` の構造を参考にしながら `DrawShadow()` を自分で作れるようになる

---

## 理論：2パス描画の全体フロー

```
【フレームごとの処理】

＜パス1：シャドウパス＞
  1. レンダーターゲットを「シャドウマップ用テクスチャ（DSV）」に切り替える
  2. シャドウ用シェーダー（ShadowMap.hlsl）をセット
  3. 影を落とすモデルをすべて描画（色は出力しない・深度だけ書く）
  4. レンダーターゲットを元の画面に戻す

＜パス2：メインパス＞
  5. 通常シェーダー（Simple3D.hlsl）をセット
  6. 普通にモデルを描画（Step5 でシャドウマップを参照して影を表示）
```

---

## この Step でのビルドチェックポイント

Step4 は変更ファイルが多いため、小さく分けてビルドしながら進めます。

```
チェックポイント A：Direct3D に BeginShadowPass / EndShadowPass を追加
チェックポイント B：Fbx に DrawShadow を追加
チェックポイント C：Model に DrawShadow を追加
チェックポイント D：Stage::Draw() を2パス構造に変更
```

---

## チェックポイント A：Direct3D への変更

### A-1. `Engine/Direct3D.h` の変更

`namespace Direct3D` の末尾（`};` の直前）に2行追加します。

**変更前（末尾付近）：**
```cpp
    // シャドウマップ用リソース
    HRESULT InitShadowMap(int width, int height);
    ID3D11ShaderResourceView* GetShadowMapSRV();
};
```

**変更後：**
```cpp
    // シャドウマップ用リソース
    HRESULT InitShadowMap(int width, int height);
    ID3D11ShaderResourceView* GetShadowMapSRV();

    void BeginShadowPass();  // シャドウ用レンダーターゲットに切り替える
    void EndShadowPass();    // 通常のレンダーターゲットに戻す
};
```

---

### A-2. `Engine/Direct3D.cpp` の変更

#### ① namespace 内に画面サイズ変数を追加

`namespace Direct3D {` の中、他のメンバ変数と並んでいる場所（`pShadowMapTexture` などの上）に追記します。

```cpp
    int screenWidth  = 0; // 画面幅（EndShadowPass でビューポートを戻すために保存）
    int screenHeight = 0; // 画面高さ
```

#### ② `Initialize()` の先頭で画面サイズを保存

`Initialize()` 関数の中の**一番最初**（`DXGI_SWAP_CHAIN_DESC` より前）に2行追記します。

```cpp
HRESULT Direct3D::Initialize(int winW, int winH, HWND hWnd)
{
    screenWidth  = winW; // ← 追加
    screenHeight = winH; // ← 追加

    // Direct3Dの初期化
    DXGI_SWAP_CHAIN_DESC scDesc = {};
    // ... 以下既存のコード（変更不要） ...
}
```

#### ③ `BeginShadowPass()` をファイル末尾に追加

```cpp
void Direct3D::BeginShadowPass()
{
    // シャドウマップの深度値を 1.0（最大値）でクリア
    pContext->ClearDepthStencilView(pShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // レンダーターゲットをシャドウマップ専用に切り替える
    // RTV = nullptr → 色は一切書かない
    // DSV = pShadowMapDSV → 深度だけシャドウマップに書く
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

    // シャドウ専用シェーダーをセット
    SetShader(SHADER_SHADOWMAP);
}
```

#### ④ `EndShadowPass()` をファイル末尾に追加

```cpp
void Direct3D::EndShadowPass()
{
    // レンダーターゲットを通常の画面（RTV + 深度ステンシル）に戻す
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

### ✅ チェックポイント A ── ここでビルド

ビルドが通ることを確認してください。  
画面の見た目はまだ変わりません。

---

## チェックポイント B：Fbx への変更

`Fbx::DrawShadow()` は `Fbx::Draw()` を参考に作ります。  
違いは以下の2点だけです。

| | `Draw()` | `DrawShadow()` |
|--|---------|----------------|
| シェーダー | `SetShader(SHADER_3D)` を自分で呼ぶ | `BeginShadowPass()` で設定済み・ここでは呼ばない |
| コンスタントバッファ | `CONSTANT_BUFFER`（WVP・マテリアルなど） | `CB_SHADOW`（matLightWVP のみ） |
| テクスチャ | セットする | **不要**（深度しか書かない） |

---

### B-1. `Engine/Fbx.h` の変更

#### ① `DrawShadow` の宣言を追加

`DrawToon` の宣言の下に1行追記します。

```cpp
void Draw(Transform& transform);
void DrawNormalMapped(Transform& transform);
void DrawToon(Transform& transform);
void DrawShadow(Transform& transform); // ← 追加：シャドウマップ生成用
```

#### ② `CB_SHADOW` 構造体を追加

`CONSTANT_BUFFER` 構造体の**下**に追記します。

```cpp
// シャドウマップ生成用コンスタントバッファ（matLightWVP だけ持てばよい）
struct CB_SHADOW
{
    XMMATRIX matLightWVP; // ライト視点の World × View × Projection
};
```

#### ③ `pShadowConstantBuffer_` メンバを追加

`pConstantBuffer_` の宣言の**下**に1行追記します。

```cpp
ID3D11Buffer* pConstantBuffer_;
ID3D11Buffer* pShadowConstantBuffer_; // ← 追加：シャドウ用CB
```

---

### B-2. `Engine/Fbx.cpp` の変更

#### ① コンストラクタ `Fbx::Fbx()` で初期化

初期化リストに `, pShadowConstantBuffer_(nullptr)` を追加します。  
`pConstantBuffer_(nullptr)` の**直後**に追記してください。

```cpp
Fbx::Fbx()
    : pVertexBuffer_(nullptr)
    , pIndexBuffer_(nullptr)
    , pConstantBuffer_(nullptr)
    , pShadowConstantBuffer_(nullptr) // ← 追加
    , vertexCount_(0)
    , polygonCount_(0)
    , materialCount_(0)
    , pToonTexture_(nullptr)
{
}
```

#### ② `Fbx::InitConstantBuffer()` でシャドウ用 CB も作成

`InitConstantBuffer()` 関数の末尾（最後の `}` の直前）に追記します。  
関数内に `HRESULT hr;` がすでにあるので、**`HRESULT hr;` は書かないでください**。

```cpp
    // シャドウマップ用コンスタントバッファ
    D3D11_BUFFER_DESC cbShadow;
    cbShadow.ByteWidth           = sizeof(CB_SHADOW);
    cbShadow.Usage               = D3D11_USAGE_DYNAMIC;
    cbShadow.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    cbShadow.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    cbShadow.MiscFlags           = 0;
    cbShadow.StructureByteStride = 0;

    hr = Direct3D::pDevice->CreateBuffer(&cbShadow, nullptr, &pShadowConstantBuffer_);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"シャドウ用コンスタントバッファの作成に失敗しました", L"エラー", MB_OK);
    }
```

#### ③ `Fbx::DrawShadow()` を追加

`DrawToon()` の**下**（`void Fbx::Release()` の前）に追記します。

```cpp
void Fbx::DrawShadow(Transform& transform)
{
    // シャドウ専用シェーダーは BeginShadowPass() で既にセット済み
    // ここでは SetShader を呼ばない

    transform.Calculation();

    // 頂点バッファをセット（Draw() と同じ）
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    Direct3D::pContext->IASetVertexBuffers(0, 1, &pVertexBuffer_, &stride, &offset);

    // ライト視点の WVP 行列を計算
    XMMATRIX matWorld    = transform.GetWorldMatrix();
    XMMATRIX matLightV   = Direct3D::GetLightViewMatrix();
    XMMATRIX matLightP   = Direct3D::GetLightProjectionMatrix();
    XMMATRIX matLightWVP = matWorld * matLightV * matLightP;

    // コンスタントバッファに書き込む
    CB_SHADOW cb;
    cb.matLightWVP = matLightWVP;

    D3D11_MAPPED_SUBRESOURCE pdata;
    Direct3D::pContext->Map(pShadowConstantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &pdata);
    memcpy_s(pdata.pData, pdata.RowPitch, (void*)(&cb), sizeof(cb));
    Direct3D::pContext->Unmap(pShadowConstantBuffer_, 0);

    // 頂点シェーダーのスロット b0 にセット（ShadowMap.hlsl の cbuffer cbShadow と対応）
    Direct3D::pContext->VSSetConstantBuffers(0, 1, &pShadowConstantBuffer_);

    // マテリアルごとにインデックスバッファをセットして描画
    // テクスチャ・マテリアル情報は深度だけの描画には不要
    for (int i = 0; i < materialCount_; i++)
    {
        stride = sizeof(int);
        offset = 0;
        Direct3D::pContext->IASetIndexBuffer(pIndexBuffer_[i], DXGI_FORMAT_R32_UINT, 0);
        Direct3D::pContext->DrawIndexed(indexCount_[i], 0, 0);
    }
}
```

### ✅ チェックポイント B ── ここでビルド

ビルドが通ることを確認してください。  
まだ `DrawShadow` はどこからも呼ばれていないので画面は変わりません。

---

## チェックポイント C：Model への変更

### C-1. `Engine/Model.h` の変更

`DrawToon` の宣言の下に1行追加します。

```cpp
void Draw(int hModel);
void DrawNormalMapped(int hModel);
void DrawToon(int hModel);
void DrawShadow(int hModel); // ← 追加
```

### C-2. `Engine/Model.cpp` の変更

`DrawToon()` の実装の**下**に追記します。

```cpp
void Model::DrawShadow(int hModel)
{
    modelList[hModel]->pfbx_->DrawShadow(modelList[hModel]->transform_);
}
```

### ✅ チェックポイント C ── ここでビルド

ビルドが通ることを確認してください。

---

## チェックポイント D：Stage::Initialize() と Stage::Draw() に変更を加える

### D-1. `Stage::Initialize()` に比較サンプラーを追加

`Stage::Initialize()` の末尾（モデルロードやカメラ設定の後）に追記します。

```cpp
// 比較サンプラー（シャドウマップ用）を作成してスロット s1 にセット
D3D11_SAMPLER_DESC sampDesc = {};
sampDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.BorderColor[0] = 1.0f; // 範囲外は「影なし」扱い
sampDesc.BorderColor[1] = 1.0f;
sampDesc.BorderColor[2] = 1.0f;
sampDesc.BorderColor[3] = 1.0f;
sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
sampDesc.MinLOD         = 0;
sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;

ID3D11SamplerState* pShadowSampler = nullptr;
Direct3D::pDevice->CreateSamplerState(&sampDesc, &pShadowSampler);
Direct3D::pContext->PSSetSamplers(1, 1, &pShadowSampler);
SAFE_RELEASE(pShadowSampler); // コンテキストが参照を保持するのでここで解放OK
```

> **なぜ Initialize() に書くのか？**  
> サンプラーは毎フレーム変わるものではないので、初期化時に1回だけ作ってセットすれば十分です。  
> `D3D11_TEXTURE_ADDRESS_BORDER` + `BorderColor = 1.0` にすることで  
> シャドウマップの UV 範囲外は「影なし（明るい）」として扱われます。

### D-2. `Stage::Draw()` を2パス構造に変更

`Stage::Draw()` の先頭（最初の `Transform ltr;` より前）に「パス1」のブロックを追加します。  
**既存のコードは削除せず、前に挿入するイメージです。**

**変更後の `Stage::Draw()` 全体：**

```cpp
void Stage::Draw()
{
    // ========================================
    // パス1：シャドウパス
    // ライト視点でシーンを描画して深度テクスチャ（シャドウマップ）を作る
    // この時点では画面には何も表示されない
    // ========================================
    Direct3D::BeginShadowPass();

    static Transform tDonut;
    tDonut.scale_    = { 0.2f, 0.2f, 0.2f };
    tDonut.position_ = { 0, 0.5f, 0.0f };
    tDonut.rotate_.y += 0.1f;
    Model::SetTransform(hDonut_, tDonut);
    Model::DrawShadow(hDonut_); // シャドウキャスター（影を落とす側）

    // hRoom_ はシャドウレシーバー（影を受ける側）なのでキャスターではない
    // 部屋の内側からライトが遮られると全面影になるため、ここには含めない
    // Model::DrawShadow(hRoom_); ← コメントアウトのまま

    Transform tr;
    tr.position_ = { 0, 0, 0 };
    tr.rotate_   = { 0, 180, 0 };

    Direct3D::EndShadowPass();

    // ========================================
    // パス2：メインパス（通常描画）
    // カメラ視点でシーンを描画する（まだ影は出ない → Step5 で追加）
    // ========================================

    // ライトの位置を示す小さなボール（パス2 のみ・影は不要）
    Transform ltr;
    ltr.position_ = { Direct3D::GetLightPos().x,
                      Direct3D::GetLightPos().y,
                      Direct3D::GetLightPos().z };
    ltr.scale_ = { 0.1f, 0.1f, 0.1f };
    Model::SetTransform(hball_, ltr);
    Model::Draw(hball_);

    Model::SetTransform(hRoom_, tr);
    Model::Draw(hRoom_);

    Model::SetTransform(hDonut_, tDonut);
    Model::Draw(hDonut_);

    // ========== ImGui でライト情報を表示 =========
    // ※ここより下は既存のコードと同じ。変更不要。
    ImGui::Text("Stage Class rot: %lf", tDonut.rotate_.z);

    ImGui::Separator();
    ImGui::Text("=== Light Type ===");
    if (ImGui::Button("Directional")) { lightType_ = 0; }
    ImGui::SameLine();
    if (ImGui::Button("Point")) { lightType_ = 1; }
    ImGui::SameLine();
    ImGui::Text("Current: %s", lightType_ == 0 ? "Directional" : "Point");

    ImGui::Separator();
    ImGui::Text("=== Light Information ===");

    XMFLOAT4 pointLight = Direct3D::GetLightPos();
    if (lightType_ == 1)
    {
        ImGui::Text("Point Light Position:");
        ImGui::Text("  X: %.2f, Y: %.2f, Z: %.2f", pointLight.x, pointLight.y, pointLight.z);
        ImGui::Text("  Control: WASD + Up/Down");
    }
    else
    {
        ImGui::Text("Directional Light Direction:");
        ImGui::Text("  X: %.2f, Y: %.2f, Z: %.2f", pointLight.x, pointLight.y, pointLight.z);
        ImGui::Text("  Control: WASD + Up/Down");
    }

    ImGui::Separator();

    // ========== Step1 デバッグ：ライト行列を表示 ==========
    if (ImGui::CollapsingHeader("Light Matrix Debug"))
    {
        XMMATRIX V = Direct3D::GetLightViewMatrix();
        XMMATRIX P = Direct3D::GetLightProjectionMatrix();
        XMMATRIX VP = V * P;

        ImGui::Text("-- LightView --");
        ImGui::Text("[0]: %.2f %.2f %.2f %.2f", V.r[0].m128_f32[0], V.r[0].m128_f32[1], V.r[0].m128_f32[2], V.r[0].m128_f32[3]);
        ImGui::Text("[1]: %.2f %.2f %.2f %.2f", V.r[1].m128_f32[0], V.r[1].m128_f32[1], V.r[1].m128_f32[2], V.r[1].m128_f32[3]);
        ImGui::Text("[2]: %.2f %.2f %.2f %.2f", V.r[2].m128_f32[0], V.r[2].m128_f32[1], V.r[2].m128_f32[2], V.r[2].m128_f32[3]);
        ImGui::Text("[3]: %.2f %.2f %.2f %.2f", V.r[3].m128_f32[0], V.r[3].m128_f32[1], V.r[3].m128_f32[2], V.r[3].m128_f32[3]);
    }
    // ========== Step1 デバッグ END ==========

    // ========== Step2 デバッグ：シャドウマップの生成確認 ==========
    ImGui::Text("ShadowMap SRV: %s",
        Direct3D::GetShadowMapSRV() != nullptr ? "OK" : "null");
    // ===============================================================
}
```

### ✅ チェックポイント D ── ここでビルドして実行

**ビルドして実行したとき、以下の状態が正解です：**

- 画面は表示される（クラッシュしない）
- 見た目はまだ Step3 までと同じ（影なし）
- フレームレートがわずかに下がっている（2パス描画の負荷増加は正常）

> **もし画面が真っ黒になった場合：**  
> `EndShadowPass()` でレンダーターゲットが正しく戻されていない可能性があります。  
> `OMSetRenderTargets` の引数 `pRenderTargetView` と `pDepthStencilView` が  
> `nullptr` になっていないか確認してください。

---

## よくある疑問

**Q. `DrawShadow()` でマテリアルのループが必要なのはなぜ？**  
A. 1つのモデルが複数のマテリアル（= 複数のインデックスバッファ）を持つ場合があるためです。  
　深度だけを書く場合でも、すべてのポリゴンを描画する必要があります。

**Q. `BeginShadowPass()` で `SetShader` を呼んでいるのに `DrawShadow()` では呼ばないのはなぜ？**  
A. シャドウパス中に描画するモデルはすべて同じシャドウシェーダーを使うので  
　`BeginShadowPass()` で1回だけセットすれば十分です。  
　`Draw()` が自分で `SetShader(SHADER_3D)` を呼んでいるのとは設計が違う点に注目しましょう。

**Q. `hball_`（ライト位置の球）をシャドウパスで描かないのはなぜ？**  
A. ライトの位置を示す目印の球が自分自身の影を落とす必要はないからです。

---

## 次のステップ

[Step5 → 影の判定をシェーダーに追加する（影が出る！）](./Step5_ShadowReceive.md)
