# 第2章：ライトを仮想カメラとして扱う

## この章の目的

シャドウマップを作るには、ライトから見た画面が必要になる。  
そのために、ライト用のビュー行列と射影行列を追加する。

この章では、まだ画面は変わらない。

---

## 学生向け説明

普通の描画では、カメラから見た画面を作っている。

```text
カメラ位置
  ↓ 見る
シーン
```

シャドウマップでは、ライトから見た画面を作る。

```text
ライト視点の仮想カメラ位置
  ↓ 見る
シーン
```

今回のライトは平行光源として扱う。  
平行光源には本来「位置」はない。

しかし、シャドウマップを作るには「ライトから見た画面」が必要なので、ライト方向の先に仮想カメラを置く。

```text
lightEye = ライト方向に離した仮想カメラ位置
lightAt  = 原点
```

ここでの原点はライト位置ではない。  
仮想ライトカメラの注視点。

---

## 変更ファイル

- `Engine/Direct3D.h`
- `Engine/Direct3D.cpp`

---

## Copilotへの指示

```text
ステンシルは使わないシャドウマップ実装の第2章です。

Beforeプロジェクトを基準に、Direct3Dにライト視点用のビュー行列と射影行列を追加してください。

変更内容：
1. Engine/Direct3D.h に次の関数宣言を追加する。
   - DirectX::XMMATRIX GetLightViewMatrix();
   - DirectX::XMMATRIX GetLightProjectionMatrix();

2. Engine/Direct3D.cpp に上記2関数を実装する。

3. GetLightViewMatrix() は、既存の lightPosition を方向ベクトルとして使う。
   - lightDir = normalize(lightPosition)
   - lightEye = lightDir * 10.0f
   - lightAt = 原点
   - XMMatrixLookAtLH(lightEye, lightAt, lightUp) を返す

4. lightDir がY軸にほぼ平行な場合は LookAt が壊れないように up をZ軸に切り替える。

5. GetLightProjectionMatrix() は平行光源用なので XMMatrixOrthographicLH を使う。
   - width = 5.0f
   - height = 5.0f
   - nearZ = 1.0f
   - farZ = 50.0f

既存の関数名・型名に合わせて実装し、不要な新規クラスは作らないでください。
```

---

## 実装イメージ

### `Engine/Direct3D.h`

```cpp
DirectX::XMMATRIX GetLightViewMatrix();
DirectX::XMMATRIX GetLightProjectionMatrix();
```

### `Engine/Direct3D.cpp`

```cpp
DirectX::XMMATRIX Direct3D::GetLightViewMatrix()
{
    XMVECTOR lightDir  = XMVector3Normalize(XMLoadFloat4(&lightPosition));
    XMVECTOR lightEye  = lightDir * 10.0f;
    XMVECTOR lightAt   = XMVectorSet(0, 0, 0, 0);

    XMVECTOR upY = XMVectorSet(0, 1, 0, 0);
    float dotY   = fabsf(XMVectorGetX(XMVector3Dot(lightDir, upY)));
    XMVECTOR lightUp = (dotY > 0.99f) ? XMVectorSet(0, 0, 1, 0) : upY;

    return XMMatrixLookAtLH(lightEye, lightAt, lightUp);
}

DirectX::XMMATRIX Direct3D::GetLightProjectionMatrix()
{
    float width  = 5.0f;
    float height = 5.0f;
    float nearZ  = 1.0f;
    float farZ   = 50.0f;

    return XMMatrixOrthographicLH(width, height, nearZ, farZ);
}
```

---

## この章でのコード変更点

### 変更の概要

| ファイル | 変更内容 |
|----------|----------|
| `Engine/Direct3D.h` | 関数宣言を2つ追加 |
| `Engine/Direct3D.cpp` | 関数の実装を2つ追加 |

画面は変わらない。まだどこからも呼ばれていないため。

---

### `Engine/Direct3D.h` の変更

#### Before（変更前）
```cpp
namespace Direct3D
{
    // ...既存の関数宣言...
    DirectX::XMFLOAT4 GetLightPos();
    void SetLightPos(DirectX::XMFLOAT4 pos);
};
```

#### After（変更後）
```cpp
namespace Direct3D
{
    // ...既存の関数宣言...
    DirectX::XMFLOAT4 GetLightPos();
    void SetLightPos(DirectX::XMFLOAT4 pos);

    DirectX::XMMATRIX GetLightViewMatrix();        // ← 追加
    DirectX::XMMATRIX GetLightProjectionMatrix();  // ← 追加
};
```

**追加した宣言の意味：**

| 関数名 | 意味 |
|--------|------|
| `GetLightViewMatrix()` | ライトを仮想カメラとして「どこから・どこを見るか」を表す行列を返す |
| `GetLightProjectionMatrix()` | ライト視点の「画面の映し方（正射影）」を表す行列を返す |

---

### `Engine/Direct3D.cpp` の変更

#### 追加した関数① `GetLightViewMatrix()`

```cpp
XMMATRIX Direct3D::GetLightViewMatrix()
{
    // lightPosition はライト方向ベクトル（平行光源のため位置ではなく向き）
    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat4(&lightPosition));

    // ライト方向の延長線上（10倍先）に仮想カメラを置く
    XMVECTOR lightEye = lightDir * 10.0f;

    // 仮想カメラはシーンの原点（0,0,0）を見る
    XMVECTOR lightAt = XMVectorSet(0, 0, 0, 0);

    // 通常は「上方向 = Y軸」でよい
    XMVECTOR upY = XMVectorSet(0, 1, 0, 0);
    float dotY   = fabsf(XMVectorGetX(XMVector3Dot(lightDir, upY)));

    // ライト方向がY軸とほぼ一致するとき（真上/真下）は、
    // LookAt の計算が壊れるため、上方向をZ軸に切り替える
    XMVECTOR lightUp = (dotY > 0.99f) ? XMVectorSet(0, 0, 1, 0) : upY;

    // ライト視点の View 行列を作って返す
    return XMMatrixLookAtLH(lightEye, lightAt, lightUp);
}
```

**ポイント：**
- `lightPosition` は平行光源なので「位置」ではなく「方向」として使う
- 方向を正規化して `* 10.0f` するだけで、仮想カメラの位置になる
- Y軸に平行なとき（`dotY > 0.99f`）だけ up をZ軸にする。これをしないと `LookAt` の計算が破綻する

---

#### 追加した関数② `GetLightProjectionMatrix()`

```cpp
XMMATRIX Direct3D::GetLightProjectionMatrix()
{
    // 平行光源は遠近感がないので「正射影（Orthographic）」を使う
    // width=5, height=5 : ライトが照らす範囲（ワールド単位）
    // nearZ=1, farZ=50  : ライト視点の手前・奥のクリップ距離
    return XMMatrixOrthographicLH(5.0f, 5.0f, 1.0f, 50.0f);
}
```

**ポイント：**
- 通常のカメラは `XMMatrixPerspectiveFovLH`（遠近感あり）を使う
- ライト視点は平行光源なので `XMMatrixOrthographicLH`（遠近感なし）を使う
- `width / height` はライトが影を作れる範囲。狭すぎると影が切れる

---

### 変更のイメージ図

```text
【変更前】
Direct3D には、カメラ用の行列しかなかった

  SetLightPos() / GetLightPos()  ← ライトの位置を持つだけ

【変更後】
ライトを「仮想カメラ」として扱う2つの行列が追加された

  GetLightViewMatrix()       ← ライトはどこから・どこを見るか
  GetLightProjectionMatrix() ← ライト視点の映し方（正射影）

  この2つを掛け合わせると「ライト視点のVP行列」になる
  → 3章以降でシャドウマップ作成に使う
```

---

## ビルド確認

- ビルドが通れば成功。
- 画面は変わらない。

---

## 推奨コミット

```bash
git add Engine/Direct3D.h Engine/Direct3D.cpp
git commit -m "Add light view projection matrices for shadow mapping"
```
