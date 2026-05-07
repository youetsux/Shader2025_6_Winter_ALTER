# Step4 ── 2パス描画に切り替える

## 何をするか

今は毎フレーム1回だけモデルを描画している。  
これを「ライト視点で描く（パス1）→ カメラ視点で描く（パス2）」の2回に増やす。

パス1では深度だけ書く。パス2は今まで通り。  
画面の見た目はまだ変わらない。

---

## 知識：2パス描画の全体フロー

```
毎フレーム
  ┌─ パス1：シャドウパス ─────────────────┐
  │ BeginShadowPass()  ← 深度テクスチャに切り替え │
  │ DrawShadow(ドーナツ) ← 深度だけ書く          │
  │ EndShadowPass()    ← 画面に戻す              │
  └───────────────────────────────────────┘
  ┌─ パス2：メインパス ─────────────────┐
  │ Draw(ボール)     ← 普通に描画               │
  │ Draw(部屋)                                   │
  │ Draw(ドーナツ)   ← ここで影の判定（Step5）   │
  └───────────────────────────────────────┘
```

---

## 実装

Step4 は変更ファイルが多いので A〜D に分けてビルドしながら進める。

---

### A：Fbx に DrawShadow を追加する

#### Engine/Fbx.h

`DrawToon` の下に宣言を追加する。

```cpp
void DrawToon(Transform& transform);
void DrawShadow(Transform& transform); // 追加
```

`CONSTANT_BUFFER` の下に構造体を追加する。

```cpp
// 追加：シャドウパス用のコンスタントバッファ（matLightWVP だけ）
struct CB_SHADOW
{
    XMMATRIX matLightWVP;
};
```

`pConstantBuffer_` の下にメンバを追加する。

```cpp
ID3D11Buffer* pConstantBuffer_;
ID3D11Buffer* pShadowConstantBuffer_; // 追加
```

#### Engine/Fbx.cpp

コンストラクタの初期化リストに追加する。

```cpp
Fbx::Fbx()
    : pVertexBuffer_(nullptr)
    , pIndexBuffer_(nullptr)
    , pConstantBuffer_(nullptr)
    , pShadowConstantBuffer_(nullptr) // 追加
    , vertexCount_(0)
    ...
```

`InitConstantBuffer()` の末尾に追加する。  
（`hr` 変数はすでに宣言されているのでそのまま使う）

```cpp
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

`DrawToon()` の下に `DrawShadow()` を追加する。  
Draw() と比べて何が違うか：シェーダーのセットなし・マテリアルなし・テクスチャなし。

```cpp
void Fbx::DrawShadow(Transform& transform)
{
    // シェーダーは BeginShadowPass() でセット済みなのでここでは不要
    transform.Calculation();

    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    Direct3D::pContext->IASetVertexBuffers(0, 1, &pVertexBuffer_, &stride, &offset);

    // ライト視点の WVP 行列を計算
    XMMATRIX matLightWVP = transform.GetWorldMatrix()
                         * Direct3D::GetLightViewMatrix()
                         * Direct3D::GetLightProjectionMatrix();

    CB_SHADOW cb;
    cb.matLightWVP = matLightWVP;

    D3D11_MAPPED_SUBRESOURCE pdata;
    Direct3D::pContext->Map(pShadowConstantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &pdata);
    memcpy_s(pdata.pData, pdata.RowPitch, (void*)(&cb), sizeof(cb));
    Direct3D::pContext->Unmap(pShadowConstantBuffer_, 0);

    Direct3D::pContext->VSSetConstantBuffers(0, 1, &pShadowConstantBuffer_);

    for (int i = 0; i < materialCount_; i++)
    {
        stride = sizeof(int);
        offset = 0;
        Direct3D::pContext->IASetIndexBuffer(pIndexBuffer_[i], DXGI_FORMAT_R32_UINT, 0);
        Direct3D::pContext->DrawIndexed(indexCount_[i], 0, 0);
    }
}
```

#### ✅ ここでビルドして確認（A）

---

### B：Model に DrawShadow を追加する

#### Engine/Model.h

```cpp
void DrawShadow(int hModel); // 追加
```

#### Engine/Model.cpp

`DrawToon()` の下に追加する。

```cpp
void Model::DrawShadow(int hModel)
{
    modelList[hModel]->pfbx_->DrawShadow(modelList[hModel]->transform_);
}
```

#### ✅ ここでビルドして確認（B）

---

### C：Stage::Initialize() に比較サンプラーを追加する

比較サンプラーは Step5 でシャドウマップを読むときに必要なもの。  
初期化時に1回作ればよいので Initialize() に書く。

#### Stage.cpp の Initialize() 末尾に追加する

```cpp
D3D11_SAMPLER_DESC sampDesc = {};
sampDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
sampDesc.BorderColor[0] = 1.0f; // 範囲外は「影なし（明るい）」扱い
sampDesc.BorderColor[1] = 1.0f;
sampDesc.BorderColor[2] = 1.0f;
sampDesc.BorderColor[3] = 1.0f;
sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
sampDesc.MinLOD         = 0;
sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;

ID3D11SamplerState* pShadowSampler = nullptr;
Direct3D::pDevice->CreateSamplerState(&sampDesc, &pShadowSampler);
Direct3D::pContext->PSSetSamplers(1, 1, &pShadowSampler);
SAFE_RELEASE(pShadowSampler); // コンテキストが参照を保持するのでここで解放 OK
```

---

### D：Stage::Draw() を2パス構造に変更する

#### Stage.h の CONSTANTBUFFER_STAGE に matLightVP を追加する

```cpp
struct CONSTANTBUFFER_STAGE
{
    XMFLOAT4   lightPosition;
    XMFLOAT4   eyePosition;
    int        lightType;
    XMFLOAT3   _pad;
    XMFLOAT4X4 matLightVP;  // 追加
};
```

#### Stage.cpp の Update() に matLightVP の計算と送信を追加する

既存の `cb._pad = { 0,0,0 };` の下に追加する。

```cpp
cb._pad = { 0,0,0 };

// 追加
XMMATRIX lightV  = Direct3D::GetLightViewMatrix();
XMMATRIX lightP  = Direct3D::GetLightProjectionMatrix();
XMMATRIX lightVP = lightV * lightP;
XMStoreFloat4x4(&cb.matLightVP, lightVP);
```

#### Stage.cpp の Draw() を2パス構造に変更する

現在の Draw() を以下に置き換える。  
パス1とパス2の境界を意識すること。

```cpp
void Stage::Draw()
{
    // ── パス1：シャドウパス ──────────────────────────────
    // ライト視点で描画して深度テクスチャ（シャドウマップ）を作る
    // 画面には何も表示されない
    Direct3D::BeginShadowPass();

    static Transform tDonut;
    tDonut.scale_    = { 0.2f, 0.2f, 0.2f };
    tDonut.position_ = { 0, 0.5f, 0.0f };
    tDonut.rotate_.y += 0.1f;
    Model::SetTransform(hDonut_, tDonut);
    Model::DrawShadow(hDonut_); // ドーナツ = シャドウキャスター（影を落とす側）

    Transform tr;
    tr.position_ = { 0, 0, 0 };
    tr.rotate_   = { 0, 180, 0 };
    // 部屋はシャドウキャスターに含めない
    // （部屋の内側からライトが遮られると全面影になる）

    Direct3D::EndShadowPass();

    // ── パス2：メインパス ─────────────────────────────────
    // カメラ視点で描画する（まだ影の判定はしない → Step5 で追加）
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

    // ── ImGui ──
    if (ImGui::Button("Directional")) { lightType_ = 0; }
    ImGui::SameLine();
    if (ImGui::Button("Point")) { lightType_ = 1; }

    XMFLOAT4 lightPos = Direct3D::GetLightPos();
    ImGui::Text("Light: %.2f, %.2f, %.2f  [WASD+UD]", lightPos.x, lightPos.y, lightPos.z);
}
```

#### ✅ ここでビルドして実行確認（D）

- 画面は表示される（クラッシュしない）
- 見た目はまだ変わらない
- もし画面が真っ黒になった場合は EndShadowPass() の OMSetRenderTargets を確認する

---

次 → [Step5](./Step5.md)
