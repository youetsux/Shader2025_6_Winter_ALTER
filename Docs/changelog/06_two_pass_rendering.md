# 変更ログ：描画を2パス構成にして matLightVP を送る

## 目的
Stage::Draw() をシャドウパスとメインパスの2回に分ける。
あわせて CONSTANTBUFFER_STAGE に matLightVP を追加して毎フレーム送る。

---

## 変更ファイル

### Stage.h

#### CONSTANTBUFFER_STAGE の変更

```cpp
// Before
struct CONSTANTBUFFER_STAGE
{
	XMFLOAT4 lightPosition;
	XMFLOAT4 eyePosition;
	int      lightType;
	XMFLOAT3 _pad;
};

// After
struct CONSTANTBUFFER_STAGE
{
	XMFLOAT4   lightPosition;
	XMFLOAT4   eyePosition;
	int        lightType;
	XMFLOAT3   _pad;
	XMFLOAT4X4 matLightVP;  // 追加
};
```

### Stage.cpp

#### Update() の変更
```cpp
// 追加（cbに値をセットする部分）
XMMATRIX lightV  = Direct3D::GetLightViewMatrix();
XMMATRIX lightP  = Direct3D::GetLightProjectionMatrix();
XMMATRIX lightVP = lightV * lightP;
XMStoreFloat4x4(&cb.matLightVP, lightVP);
```

#### Draw() の変更

```cpp
// Before
void Stage::Draw()
{
	Model::Draw(hball_);
	Model::Draw(hRoom_);
	Model::Draw(hDonut_);
}

// After
void Stage::Draw()
{
	// パス1：シャドウパス
	Direct3D::BeginShadowPass();
	Model::DrawShadow(hDonut_);
	Direct3D::EndShadowPass();

	// パス2：メインパス
	Model::Draw(hball_);
	Model::Draw(hRoom_);
	Model::Draw(hDonut_);
}
```

---

## 元に戻す方法

```bash
git checkout HEAD~1 -- Stage.h Stage.cpp
```

---

## 対応するコミット

chapter6: 描画を2パス構成にして matLightVP を送る
