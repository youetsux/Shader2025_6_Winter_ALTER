# 第5章：モデルをシャドウマップに描けるようにする

## この章の目的

通常描画用の `Draw()` とは別に、シャドウマップ作成用の `DrawShadow()` を追加する。

この章でも画面は変わらない。

---

## 学生向け説明

普通の `Draw()` は、画面に表示するための描画。

```text
Draw()
  色を出す
  テクスチャを読む
  ライト計算をする
  カメラ視点で描く
```

シャドウマップ用の描画では、色はいらない。

```text
DrawShadow()
  色を出さない
  テクスチャを読まない
  ライト計算もしない
  ライト視点のWVPだけ使う
```

---

## 変更ファイル

- `Engine/Fbx.h`
- `Engine/Fbx.cpp`
- `Engine/Model.h`
- `Engine/Model.cpp`

---

## Copilotへの指示

```text
ステンシルは使わないシャドウマップ実装の第5章です。

FbxとModelに、シャドウマップ生成用の DrawShadow を追加してください。

変更内容：
1. Engine/Fbx.h に void DrawShadow(Transform& transform); を追加する。

2. Engine/Fbx.h の private に、シャドウ用コンスタントバッファ構造体を追加する。
   - struct CB_SHADOW { XMMATRIX matLightWVP; };

3. Engine/Fbx.h に ID3D11Buffer* pShadowConstantBuffer_; を追加する。

4. Engine/Fbx.cpp の Fbx コンストラクタで pShadowConstantBuffer_ を nullptr 初期化する。

5. Engine/Fbx.cpp の InitConstantBuffer() で、pShadowConstantBuffer_ を作成する。
   - ByteWidth = sizeof(CB_SHADOW)
   - Usage = D3D11_USAGE_DYNAMIC
   - BindFlags = D3D11_BIND_CONSTANT_BUFFER
   - CPUAccessFlags = D3D11_CPU_ACCESS_WRITE

6. Engine/Fbx.cpp に DrawShadow(Transform& transform) を実装する。
   - transform.Calculation() を呼ぶ。
   - 頂点バッファをセットする。
   - matLightWVP = World * Direct3D::GetLightViewMatrix() * Direct3D::GetLightProjectionMatrix() を計算する。
   - pShadowConstantBuffer_ に CB_SHADOW を Map / memcpy_s / Unmap で送る。
   - VSSetConstantBuffers(0, 1, &pShadowConstantBuffer_) でセットする。
   - マテリアルごとのインデックスバッファをセットして DrawIndexed する。
   - テクスチャやマテリアル色は使わない。

7. Engine/Model.h に void DrawShadow(int hModel); を追加する。

8. Engine/Model.cpp に Model::DrawShadow(int hModel) を追加し、内部で Fbx::DrawShadow を呼ぶ。

既存の Draw() を壊さないでください。
```

---

## 実装の中心

```cpp
XMMATRIX matLightWVP = transform.GetWorldMatrix()
                      * Direct3D::GetLightViewMatrix()
                      * Direct3D::GetLightProjectionMatrix();
```

これは、モデルの頂点をライト視点に変換するための行列。

```text
モデル座標
↓ World
ワールド座標
↓ Light View
ライトから見た座標
↓ Light Projection
ライト画面上の座標
```

---

## この章でのコード変更点

### この章でやること

シャドウマップにモデルを描くための関数 `DrawShadow()` を追加する。  
既存の `Draw()` は一切変更しない。新しい関数を追加するだけ。

---

### 変更の概要

| ファイル | 変更内容 |
|----------|----------|
| `Engine/Fbx.h` | `DrawShadow()` 宣言・`CB_SHADOW` 構造体・`pShadowConstantBuffer_` 追加 |
| `Engine/Fbx.cpp` | コンストラクタ初期化・`InitConstantBuffer()` 修正・`DrawShadow()` 実装 |
| `Engine/Model.h` | `DrawShadow(int hModel)` 宣言追加 |
| `Engine/Model.cpp` | `DrawShadow()` 実装 |

画面は変わらない。まだ `DrawShadow()` はどこからも呼ばれていないため。

---

### `Engine/Fbx.h` の変更

#### ここでやること
`DrawShadow()` 関数の宣言と、それに必要なバッファの定義を追加する。

#### `CB_SHADOW` 構造体をなぜ別に作るか

既存の `CONSTANT_BUFFER` はこれだけのデータを持っている：

```cpp
struct CONSTANT_BUFFER
{
    XMMATRIX matWVP;      // カメラ視点のWVP
    XMMATRIX matWorld;    // ワールド行列
    XMMATRIX matNormal;   // 法線変換行列
    XMFLOAT4 diffuse;     // 色
    // ...（たくさん）
};
```

シャドウマップでは**ライト視点のWVPしか要らない**。  
色も法線も何もいらない。

```cpp
struct CB_SHADOW
{
    XMMATRIX matLightWVP;  // これだけ
};
```

色やテクスチャを送らない分、軽くて速い。

#### `pShadowConstantBuffer_` をなぜ別に持つか

`pConstantBuffer_`（通常描画用）を使い回せばいいと思うかもしれないが、それはできない。

```text
理由：バッファのサイズが違う

pConstantBuffer_     → sizeof(CONSTANT_BUFFER)  大きい
pShadowConstantBuffer_ → sizeof(CB_SHADOW)       小さい

サイズが違うバッファを使い回すとエラーになる。
```

だから**シャドウ用のバッファを別に作る**。

#### After（変更後）

```cpp
class Fbx
{
public:
    // ...
    void DrawShadow(Transform& transform);  // ← 追加

private:
    // ...
    struct CB_SHADOW
    {
        XMMATRIX matLightWVP;  // ← 追加：ライト視点のWVP行列
    };

    ID3D11Buffer* pShadowConstantBuffer_;  // ← 追加：シャドウ用バッファ
};
```

---

### `Engine/Fbx.cpp` の変更

#### ここでやること
① コンストラクタで `pShadowConstantBuffer_` を nullptr 初期化する。  
② `InitConstantBuffer()` でシャドウ用バッファを作成する。  
③ `DrawShadow()` を実装する。

#### ① コンストラクタの初期化

```cpp
Fbx::Fbx()
{
    // ...既存の初期化...
    pShadowConstantBuffer_ = nullptr;  // ← 追加
}
```

#### ② `InitConstantBuffer()` への追加

```cpp
// シャドウ用コンスタントバッファを作成する
D3D11_BUFFER_DESC cbd = {};
cbd.ByteWidth      = sizeof(CB_SHADOW);           // CB_SHADOWのサイズ
cbd.Usage          = D3D11_USAGE_DYNAMIC;          // CPUから毎フレーム書き換える
cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;   // コンスタントバッファとして使う
cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;       // CPUが書き込める
Direct3D::pDevice->CreateBuffer(&cbd, nullptr, &pShadowConstantBuffer_);
```

#### ③ `DrawShadow()` の実装

```cpp
void Fbx::DrawShadow(Transform& transform)
{
    // ワールド行列を計算する
    transform.Calculation();

    // 頂点バッファをセットする（通常のDrawと同じ）
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    Direct3D::pContext->IASetVertexBuffers(0, 1, &pVertexBuffer_, &stride, &offset);

    // ライト視点のWVP行列を作る
    // モデル座標 → ワールド → ライト視点 → ライト画面
    XMMATRIX matLightWVP = transform.GetWorldMatrix()
                         * Direct3D::GetLightViewMatrix()
                         * Direct3D::GetLightProjectionMatrix();

    // CB_SHADOW にデータを詰めてGPUに送る
    CB_SHADOW cb;
    cb.matLightWVP = matLightWVP;

    // Map：GPUのバッファをCPUから書き込めるように開く
    D3D11_MAPPED_SUBRESOURCE pdata;
    Direct3D::pContext->Map(pShadowConstantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &pdata);
    // memcpy_s：データをコピーする
    memcpy_s(pdata.pData, pdata.RowPitch, &cb, sizeof(cb));
    // Unmap：書き込みを終了してGPUに返す
    Direct3D::pContext->Unmap(pShadowConstantBuffer_, 0);

    // バッファを頂点シェーダーの b0 スロットにセット
    Direct3D::pContext->VSSetConstantBuffers(0, 1, &pShadowConstantBuffer_);

    // マテリアルごとに描画する（色・テクスチャは使わない）
    for (int i = 0; i < materialCount_; i++)
    {
        Direct3D::pContext->IASetIndexBuffer(pIndexBuffer_[i], DXGI_FORMAT_R32_UINT, 0);
        Direct3D::pContext->DrawIndexed(indexCount_[i], 0, 0);
    }
}
```

---

### `Engine/Model.h` / `Engine/Model.cpp` の変更

#### ここでやること
`Fbx::DrawShadow()` を外から呼べるように `Model::DrawShadow()` を追加する。

```cpp
// Model.h に追加
void DrawShadow(int hModel);

// Model.cpp に追加
void Model::DrawShadow(int hModel)
{
    pFbx[hModel]->DrawShadow(...);
}
```

`Stage.cpp` からは `Model::DrawShadow(hDonut_)` のように呼ぶ（6章で追加）。

---

## ビルド確認

- ビルドが通れば成功。
- 画面は変わらない。
- まだ `DrawShadow()` は呼ばれていないため、見た目に変化はない。

---

## 推奨コミット

```bash
git add Engine/Fbx.h Engine/Fbx.cpp Engine/Model.h Engine/Model.cpp
git commit -m "Add DrawShadow path for FBX models"
```
