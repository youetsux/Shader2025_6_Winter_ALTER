# 変更ログ：DrawShadow と CB_SHADOW を追加

## 目的
シャドウマップにモデルを描くための DrawShadow() を、既存の Draw() を壊さずに追加する。

---

## 変更ファイル

### Engine/Fbx.h

#### 追加した宣言・構造体・メンバー変数
```cpp
// public
void DrawShadow(Transform& transform);

// private（構造体）
struct CB_SHADOW
{
	DirectX::XMMATRIX matLightWVP;
};

// private（メンバー変数）
ID3D11Buffer* pShadowConstantBuffer_;
```

### Engine/Fbx.cpp

#### Fbx コンストラクタの変更
```cpp
pShadowConstantBuffer_ = nullptr;
```

#### InitConstantBuffer() の変更
- `pShadowConstantBuffer_` を以下の設定で作成
  - `ByteWidth = sizeof(CB_SHADOW)`
  - `Usage = D3D11_USAGE_DYNAMIC`
  - `BindFlags = D3D11_BIND_CONSTANT_BUFFER`
  - `CPUAccessFlags = D3D11_CPU_ACCESS_WRITE`

#### DrawShadow() の実装内容
```cpp
void Fbx::DrawShadow(Transform& transform)
{
	transform.Calculation();
	// 頂点バッファをセット
	XMMATRIX matLightWVP = transform.GetWorldMatrix()
						 * Direct3D::GetLightViewMatrix()
						 * Direct3D::GetLightProjectionMatrix();
	// CB_SHADOW を Map / memcpy_s / Unmap で送る
	// VSSetConstantBuffers(0, 1, &pShadowConstantBuffer_)
	// マテリアルごとに DrawIndexed
}
```

### Engine/Model.h

#### 追加した宣言
```cpp
void DrawShadow(int hModel);
```

### Engine/Model.cpp

#### 追加した実装
```cpp
void Model::DrawShadow(int hModel)
{
	// Fbx::DrawShadow に転送
}
```

---

## 元に戻す方法

```bash
git checkout HEAD~1 -- Engine/Fbx.h Engine/Fbx.cpp Engine/Model.h Engine/Model.cpp
```

---

## 対応するコミット

chapter5: DrawShadow と CB_SHADOW を追加
