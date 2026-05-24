# 第8章：影が出ないときの確認ポイント

## 目的

学生が詰まりやすいところを、症状別に確認できるようにする。

---

## 1. 影がまったく出ない

### 確認すること

- `Stage::Draw()` で `Direct3D::BeginShadowPass()` を呼んでいるか。
- `Model::DrawShadow(hDonut_)` を呼んでいるか。
- `Direct3D::EndShadowPass()` を呼んでいるか。
- `Stage::Draw()` のメインパス前に `PSSetShaderResources(1, 1, &pShadowSRV)` を呼んでいるか。
- `Simple3D.hlsl` に `g_shadowMap : register(t1)` があるか。
- `Simple3D.hlsl` に `g_shadowSampler : register(s1)` があるか。
- `Stage::Update()` で `matLightVP` を送っているか。

---

## 2. 画面全体が暗くなる

### 原因候補

`hRoom_` を `DrawShadow()` している可能性が高い。

### 理由

部屋全体をシャドウパスに描くと、部屋の壁や天井がライトを遮って、室内全体が影になることがある。

### 対応

最初の授業では、シャドウパスにはドーナツだけを描く。

```cpp
Model::DrawShadow(hDonut_);
// Model::DrawShadow(hRoom_); は呼ばない
```

---

## 3. ドーナツの表面がザラザラする

### 原因候補

自分自身に影を落としている。  
これをシャドウアクネという。

### 対応

`Simple3D.hlsl` の bias を少し大きくする。

```hlsl
float bias = 0.005;
```

例えば、次のように試す。

```hlsl
float bias = 0.007;
```

ただし、大きくしすぎると影が浮いたり消えたりする。

---

## 4. 影が消える、または薄すぎる

### 確認すること

- `bias` が大きすぎないか。
- `GetLightProjectionMatrix()` の正射影サイズが広すぎないか。
- シャドウマップ解像度が低すぎないか。

この教材ではまず次の値を使う。

```cpp
XMMatrixOrthographicLH(5.0f, 5.0f, 1.0f, 50.0f);
```

---

## 5. 影の位置がライト方向と合わない

### 確認すること

- `GetLightViewMatrix()` の `lightEye` の向き。
- `Simple3D.hlsl` のライト方向 `lightPosition` の扱い。
- `matLightVP = lightV * lightP` の順番。
- `DrawShadow()` の `matLightWVP = World * LightView * LightProjection` の順番。

今回の完成形では、`lightEye` は `lightDir * 10.0f` とする。

```cpp
XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat4(&lightPosition));
XMVECTOR lightEye = lightDir * 10.0f;
```

---

## 6. 次フレームで警告や描画崩れが出る

### 原因候補

シャドウマップをSRVにセットしたまま、次のフレームでDSVとして使おうとしている。

### 対応

メインパスの後で必ずSRVを解除する。

```cpp
ID3D11ShaderResourceView* nullSRV = nullptr;
Direct3D::pContext->PSSetShaderResources(1, 1, &nullSRV);
```

---

## 7. この教材で使わないもの

この教材ではステンシルは使わない。

```text
ステンシルで影の領域を作る
```

のではなく、

```text
Simple3D.hlsl のピクセルシェーダーで深度比較する
```

方式に統一する。
