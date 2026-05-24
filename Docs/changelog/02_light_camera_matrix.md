# 変更ログ：ライト視点のビュー行列・射影行列を追加

## 目的
シャドウマップを作るために、ライト視点の仮想カメラ行列を Direct3D に追加する。

---

## 変更ファイル

### Engine/Direct3D.h

#### 追加した宣言
```cpp
DirectX::XMMATRIX GetLightViewMatrix();
DirectX::XMMATRIX GetLightProjectionMatrix();
```

### Engine/Direct3D.cpp

#### 追加した実装

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
	return XMMatrixOrthographicLH(5.0f, 5.0f, 1.0f, 50.0f);
}
```

---

## 元に戻す方法

```bash
git checkout HEAD~1 -- Engine/Direct3D.h Engine/Direct3D.cpp
```

---

## 対応するコミット

chapter2: ライト視点のビュー行列・射影行列を追加
