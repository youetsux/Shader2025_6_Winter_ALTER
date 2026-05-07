# Step1 ── ライト視点の行列を追加する

## 何をするか

シャドウマップでは「ライトの目線からシーンを描画する」ことが必要になる。  
そのためにカメラと同じように、ライト用のビュー行列と射影行列を用意する。

この Step では関数を2つ追加するだけ。画面は変わらない。

---

## 知識：カメラの行列とライトの行列は同じもの

今の描画では、カメラのビュー行列と射影行列を使ってこう計算している。

```
頂点のスクリーン座標 = 頂点座標 × ワールド行列 × ビュー行列 × 射影行列
```

シャドウマップでも全く同じことをする。カメラの代わりにライトを置くだけ。

```
ライト視点の座標 = 頂点座標 × ワールド行列 × ライトビュー行列 × ライト射影行列
```

---

## 知識：平行光源には正射影を使う

カメラには遠くが小さく見える「透視投影」を使っているが、  
太陽光のような平行光源は遠くても近くても同じ角度で光が来る。  
なので距離で大きさが変わらない「正射影」を使う。

```
透視投影 XMMatrixPerspectiveFovLH  → カメラ・点光源
正射影   XMMatrixOrthographicLH   → 平行光源のシャドウ ← これを使う
```

---

## 知識：lightPosition の意味

`Simple3D.hlsl` の PS() を見ると

```hlsl
L = normalize(lightPosition.xyz);
float ndotl = dot(N, L);
```

`lightPosition` は「光が来る方向を指す矢印」として使っている。  
シャドウマップでもこの値をそのまま使う。

ライトに位置はないので、方向の延長線上に仮想的な位置を置く。

```
lightDir = normalize(lightPosition)
lightEye = lightDir * 10   // その方向に 10 だけ離れた場所 = 仮想ライト位置
lightAt  = (0, 0, 0)       // 注視点はシーンの中心（原点）
```

---

## 実装

### Engine/Direct3D.h

`SetLightPos` の下に2行追加する。

```cpp
DirectX::XMFLOAT4 GetLightPos();
void SetLightPos(DirectX::XMFLOAT4 pos);

DirectX::XMMATRIX GetLightViewMatrix();       // 追加
DirectX::XMMATRIX GetLightProjectionMatrix(); // 追加
```

### Engine/Direct3D.cpp

`SetLightPos` の定義の下に追記する。

```cpp
DirectX::XMMATRIX Direct3D::GetLightViewMatrix()
{
    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat4(&lightPosition));
    XMVECTOR lightEye = lightDir * 10.0f;
    XMVECTOR lightAt  = XMVectorSet(0, 0, 0, 0);

    // lightDir が真上を向いているとき LookAt が壊れるので上方向を切り替える
    XMVECTOR upY = XMVectorSet(0, 1, 0, 0);
    float dotY   = fabsf(XMVectorGetX(XMVector3Dot(lightDir, upY)));
    XMVECTOR lightUp = (dotY > 0.99f) ? XMVectorSet(0, 0, 1, 0) : upY;

    return XMMatrixLookAtLH(lightEye, lightAt, lightUp);
}

DirectX::XMMATRIX Direct3D::GetLightProjectionMatrix()
{
    return XMMatrixOrthographicLH(5.0f, 5.0f, 1.0f, 50.0f);
}
```

---

## ビルドして確認

ビルドが通れば OK。画面は変わらない。

次 → [Step2](./Step2.md)
