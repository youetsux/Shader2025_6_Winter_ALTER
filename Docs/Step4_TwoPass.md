# Step 4 ── 2パス描画を組み込む

## 学習目標

- 「同じモデルを2回描く」という2パス描画の構造を理解する
- レンダーターゲットの切り替え（シャドウ用 ↔ 通常）の方法を知る
- パス1（シャドウパス）とパス2（メインパス）を `Stage::Draw()` で明確に分離する

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

パス1とパス2は**描画先（レンダーターゲット）が違うだけ**で、  
描いているモデルは同じです。

---

## 変更内容

### 変更ファイル：`Engine/Direct3D.h`

シャドウパスの開始・終了関数を追加します。

```cpp
// 2パス描画用
void BeginShadowPass();  // シャドウ用レンダーターゲットに切り替える
void EndShadowPass();    // 通常のレンダーターゲットに戻す
```

---

### 変更ファイル：`Engine/Direct3D.cpp`

#### ① `BeginShadowPass()` を追加

```cpp
void Direct3D::BeginShadowPass()
{
    // シャドウマップ DSV をクリア（深度を最大値 1.0 でリセット）
    pContext->ClearDepthStencilView(pShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // レンダーターゲットを「シャドウマップ用（色なし・深度のみ）」に切り替える
    // 第1引数: RTV=nullptr → 色は一切書かない
    // 第2引数: DSV=pShadowMapDSV → 深度だけシャドウマップに書く
    ID3D11RenderTargetView* nullRTV = nullptr;
    pContext->OMSetRenderTargets(1, &nullRTV, pShadowMapDSV);

    // シャドウマップのサイズに合わせてビューポートを設定
    D3D11_TEXTURE2D_DESC texDesc;
    pShadowMapTexture->GetDesc(&texDesc);

    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)texDesc.Width;
    vp.Height   = (float)texDesc.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    pContext->RSSetViewports(1, &vp);

    // シャドウ用シェーダーをセット
    SetShader(SHADER_SHADOWMAP);
}
```

#### ② `EndShadowPass()` を追加

```cpp
void Direct3D::EndShadowPass()
{
    // レンダーターゲットを通常の画面（RTV + 深度ステンシル）に戻す
    pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);

    // ビューポートも画面サイズに戻す（BeginDraw と同じ設定）
    // ※ winW, winH を保存しておく必要がある（下記参照）
    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)screenWidth;  // ← Initialize() で保存した画面幅
    vp.Height   = (float)screenHeight; // ← Initialize() で保存した画面高さ
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    pContext->RSSetViewports(1, &vp);
}
```

> **注意：** `EndShadowPass()` では画面サイズが必要です。  
> `namespace Direct3D` 内に `int screenWidth, screenHeight;` を追加して、  
> `Initialize()` の中で `screenWidth = winW; screenHeight = winH;` と保存してください。

#### ③ `namespace Direct3D` に画面サイズ変数を追加

```cpp
namespace Direct3D
{
    // ... 既存のメンバ ...

    int screenWidth  = 0; // ← 追加
    int screenHeight = 0; // ← 追加
}
```

#### ④ `Initialize()` で画面サイズを保存

```cpp
HRESULT Direct3D::Initialize(int winW, int winH, HWND hWnd)
{
    screenWidth  = winW; // ← 追加
    screenHeight = winH; // ← 追加

    // ... 以下既存のコード ...
}
```

---

### 変更ファイル：`Engine/Model.h`

シャドウパス用の描画関数を追加します。

```cpp
void DrawShadow(int hModel); // シャドウマップ生成用（深度のみ描画）
```

---

### 変更ファイル：`Engine/Model.cpp`

`DrawShadow()` の実装を追加します。  
内部では `Fbx::Draw()` と同じ仕組みを使いますが、  
シャドウ用コンスタントバッファ（`matLightWVP`）を送る必要があります。

```cpp
void Model::DrawShadow(int hModel)
{
    modelList[hModel]->pfbx_->DrawShadow(modelList[hModel]->transform_);
}
```

> **注意：** `Fbx::DrawShadow()` は Step4 で追加が必要です。  
> `Fbx.h / Fbx.cpp` に以下を追加してください。

---

### 変更ファイル：`Engine/Fbx.h`

```cpp
void DrawShadow(Transform& transform); // シャドウマップ用描画
```

---

### 変更ファイル：`Engine/Fbx.cpp`

```cpp
void Fbx::DrawShadow(Transform& transform)
{
    // ライト視点の WVP 行列を計算
    transform.Calculation();
    XMMATRIX matWorld = transform.GetWorldMatrix();
    XMMATRIX matLightV = Direct3D::GetLightViewMatrix();
    XMMATRIX matLightP = Direct3D::GetLightProjectionMatrix();
    XMMATRIX matLightWVP = matWorld * matLightV * matLightP;

    // シャドウ用コンスタントバッファ構造体
    struct CB_SHADOW {
        XMMATRIX matLightWVP;
    };

    // コンスタントバッファを作成 & 送信
    // ※ 毎フレーム作るのは非効率。本来は事前に作成しておくべき（Step6 で改善）
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth      = sizeof(CB_SHADOW);
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer* pCB = nullptr;
    Direct3D::pDevice->CreateBuffer(&bd, nullptr, &pCB);

    D3D11_MAPPED_SUBRESOURCE mapped;
    Direct3D::pContext->Map(pCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    CB_SHADOW cb;
    cb.matLightWVP = XMMatrixTranspose(matLightWVP);
    memcpy(mapped.pData, &cb, sizeof(cb));
    Direct3D::pContext->Unmap(pCB, 0);

    Direct3D::pContext->VSSetConstantBuffers(0, 1, &pCB);

    // 頂点バッファ・インデックスバッファをセットして描画（既存の Draw と同様）
    // ※ Fbx 内部の描画処理を再利用
    DrawInternal(); // ← Fbx の内部描画関数（下記参照）

    pCB->Release();
}
```

> **補足：** `DrawInternal()` は既存の `Draw()` から「シェーダーのセット部分を除いた」  
> 頂点バッファ・インデックスバッファのセットと `DrawIndexed()` の部分です。  
> `Fbx.cpp` の実装を確認しながら調整してください。

---

### 変更ファイル：`Stage.cpp`

`Stage::Draw()` の構造を2パスに分けます。

**変更前（現在）：**
```cpp
void Stage::Draw()
{
    // ... モデルのセットと描画 ...
    Model::Draw(hDonut_);
    Model::Draw(hRoom_);
}
```

**変更後：**
```cpp
void Stage::Draw()
{
    // ========================================
    // パス1：シャドウパス
    // ライト視点でシーンを描画して深度テクスチャを作る
    // ========================================
    Direct3D::BeginShadowPass();

    static Transform tDonut;
    tDonut.scale_    = { 0.2f, 0.2f, 0.2f };
    tDonut.position_ = { 0, 0.5f, 0.0f };
    tDonut.rotate_.y += 0.1f;
    Model::SetTransform(hDonut_, tDonut);
    Model::DrawShadow(hDonut_); // 影を落とすモデルをシャドウパスで描く

    Transform tr;
    tr.position_ = { 0, 0, 0 };
    tr.rotate_   = { 0, 180, 0 };
    Model::SetTransform(hRoom_, tr);
    Model::DrawShadow(hRoom_); // 部屋も影を落とすなら描く

    Direct3D::EndShadowPass();

    // ========================================
    // パス2：メインパス（通常描画）
    // ========================================
    Direct3D::SetShader(SHADER_3D);

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

    // ImGui ...（既存のまま）
}
```

---

## 確認方法

1. ビルドして実行する
2. 画面の見た目はまだ変わりません（影判定はStep5で追加）
3. フレームレートが半分程度になっていないか確認する  
   → 2パスなので多少の負荷増加は正常

> **ポイント：** パス1では `RTV = nullptr` にしているため色は一切書きません。  
> でも深度バッファ（`pShadowMapDSV`）には書かれています。  
> Step6 でこの中身を可視化して確認します。

---

## よくある疑問

**Q. なぜ `tDonut` を `static` にするの？**  
A. パス1とパス2で**同じ位置・回転**のモデルを描く必要があります。  
　`static` にすることでパス1の回転結果がパス2でもそのまま使えます。

**Q. ライトを表示する `hball_` はシャドウパスで描かなくていいの？**  
A. ライトの位置を示す小さなボールが自分自身の影を落とす必要はないので、  
　パス2だけで描けば十分です。

---

## 次のステップ

[Step5 → 影の判定をシェーダーに追加する](./Step5_ShadowReceive.md)
