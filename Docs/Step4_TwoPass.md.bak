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

`namespace Direct3D` の関数宣言に以下を追加します。

**変更前（末尾付近）：**
```cpp
DirectX::XMMATRIX GetLightViewMatrix();
DirectX::XMMATRIX GetLightProjectionMatrix();
```

**変更後：**
```cpp
DirectX::XMMATRIX GetLightViewMatrix();
DirectX::XMMATRIX GetLightProjectionMatrix();

void BeginShadowPass();  // シャドウ用レンダーターゲットに切り替える
void EndShadowPass();    // 通常のレンダーターゲットに戻す
```

### A-2. `Engine/Direct3D.cpp` の変更

#### ① namespace 内に画面サイズ変数を追加

`namespace Direct3D` の `{` の中（他のメンバ変数と並べて）に追記します。

```cpp
int screenWidth  = 0; // 画面幅（EndShadowPass でビューポートを戻すために保存）
int screenHeight = 0; // 画面高さ
```

#### ② `Initialize()` の先頭で画面サイズを保存

`Initialize()` 関数の一番最初に追記します。

```cpp
HRESULT Direct3D::Initialize(int winW, int winH, HWND hWnd)
{
    screenWidth  = winW; // ← 追加
    screenHeight = winH; // ← 追加

    // ... 以下既存のコード ...
}
```

#### ③ `BeginShadowPass()` を追加

ファイル末尾に追記します。

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

#### ④ `EndShadowPass()` を追加

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
| シェーダー | `SHADER_3D` | `BeginShadowPass()` で設定済み・ここでは呼ばない |
| コンスタントバッファ | `CONSTANT_BUFFER`（WVP・マテリアルなど） | `CB_SHADOW`（matLightWVP のみ） |
| テクスチャ | セットする | **不要**（深度しか書かない） |

### B-1. `Engine/Fbx.h` の変更

#### ① `DrawShadow` の宣言を追加

```cpp
void Draw(Transform& transform);
void DrawNormalMapped(Transform& transform);
void DrawToon(Transform& transform);
void DrawShadow(Transform& transform); // ← 追加：シャドウマップ生成用
```

#### ② `CB_SHADOW` 構造体を追加

`CONSTANT_BUFFER` 構造体の下に追記します。

```cpp
// シャドウマップ生成用コンスタントバッファ（matLightWVP だけ持てばよい）
struct CB_SHADOW
{
    XMMATRIX matLightWVP; // ライト視点の World × View × Projection
};
```

#### ③ `pShadowConstantBuffer_` メンバを追加

`pConstantBuffer_` の宣言の下に追記します。

```cpp
ID3D11Buffer* pConstantBuffer_;
ID3D11Buffer* pShadowConstantBuffer_; // ← 追加：シャドウ用CB
```

### B-2. `Engine/Fbx.cpp` の変更

#### ① コンストラクタ `Fbx::Fbx()` で初期化

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

既存の `pConstantBuffer_` を作っている `InitConstantBuffer()` 関数の末尾に追記します。

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

> **注意：** `InitConstantBuffer()` 内の `hr` 変数はすでに宣言されているはずです。  
> 新たに `HRESULT hr;` を宣言しないようにしてください。

#### ③ `Fbx::DrawShadow()` を追加

`DrawToon()` の下に追記します。  
`Draw()` と見比べながら、何が省略されているか意識してください。

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

```cpp
void Draw(int hModel);
void DrawNormalMapped(int hModel);
void DrawToon(int hModel);
void DrawShadow(int hModel); // ← 追加
```

### C-2. `Engine/Model.cpp` の変更

`DrawToon()` の下に追記します。

```cpp
void Model::DrawShadow(int hModel)
{
    modelList[hModel]->pfbx_->DrawShadow(modelList[hModel]->transform_);
}
```

### ✅ チェックポイント C ── ここでビルド

ビルドが通ることを確認してください。

---

## チェックポイント D：Stage::Draw() を2パス構造に変更

`Stage::Draw()` を以下のように変更します。  
パス1とパス2の境界を**コメントで明確に分ける**ことがポイントです。

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
    Model::DrawShadow(hDonut_);

    Transform tr;
    tr.position_ = { 0, 0, 0 };
    tr.rotate_   = { 0, 180, 0 };
    Model::SetTransform(hRoom_, tr);
    Model::DrawShadow(hRoom_);

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

    // ========== ImGui（既存のまま） ==========
    ImGui::Text("Stage Class rot: %lf", tDonut.rotate_.z);
    ImGui::Separator();
    ImGui::Text("=== Light Type ===");
    if (ImGui::Button("Directional")) { lightType_ = 0; }
    ImGui::SameLine();
    if (ImGui::Button("Point"))       { lightType_ = 1; }
    ImGui::SameLine();
    ImGui::Text("Current: %s", lightType_ == 0 ? "Directional" : "Point");
    ImGui::Separator();
    ImGui::Text("=== Light Information ===");
    XMFLOAT4 pointLight = Direct3D::GetLightPos();
    if (lightType_ == 1)
    {
        ImGui::Text("Point Light Position:");
    }
    else
    {
        ImGui::Text("Directional Light Direction:");
    }
    ImGui::Text("  X: %.2f, Y: %.2f, Z: %.2f", pointLight.x, pointLight.y, pointLight.z);
    ImGui::Text("  Control: WASD + Up/Down");
    ImGui::Separator();
}
```

### ✅ チェックポイント D ── ここでビルドして実行

**ビルドして実行したとき、以下の状態が正解です：**

- 画面は表示される（クラッシュしない）
- 見た目はまだ Step3 までと同じ（影なし）
- フレームレートがわずかに下がっている（2パス描画の負荷増加は正常）

> **もし画面が真っ黒になった場合：**  
> `EndShadowPass()` のレンダーターゲットが正しく戻されていない可能性があります。  
> `Direct3D::pRenderTargetView` と `Direct3D::pDepthStencilView` が  
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
