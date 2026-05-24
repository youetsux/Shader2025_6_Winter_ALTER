# 変更ログ：デフォルトライトを平行光源に変更

## 目的
点光源のみだった Simple3D.hlsl のライト計算を、平行光源（lightType=0）と点光源（lightType=1）の
両対応にし、デフォルトを平行光源にする。

---

## 変更ファイル

### Simple3D.hlsl

#### cbuffer gStage の変更前
```hlsl
cbuffer gStage : register(b1)
{
	float4 lightPosition;
	float4 eyePosition;
};
```

#### cbuffer gStage の変更後
```hlsl
cbuffer gStage : register(b1)
{
	float4 lightPosition;
	float4 eyePosition;
	int    lightType;  // 0=平行光源, 1=点光源
	float3 _pad;
};
```

#### PS の変更
- `lightType == 0`（平行光源）: `lightPosition.xyz` をそのままライト方向として使用、距離減衰なし
- `lightType == 1`（点光源）: 従来通り位置からベクトルを計算、距離減衰あり

### Stage.cpp（変更なし）
- コンストラクタで `lightType_ = 0`（平行光源）に初期化済み

---

## 元に戻す方法

```bash
git checkout HEAD~1 -- Simple3D.hlsl
```

---

## 対応するコミット

`feat: Simple3D.hlsl を平行光源デフォルトに変更（lightType対応）`
