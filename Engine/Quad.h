#pragma once
#include "Direct3D.h"
#include <DirectXMath.h>

using namespace DirectX;

struct CONSTANT_BUFFER
{
	DirectX::XMMATRIX matWVP; //world*view*projectionの行列
	DirectX::XMMATRIX matNormal; //法線変換用の行列
	DirectX::XMMATRIX matWorld; //ワールド行列
};

struct VERTEX
{
	XMFLOAT4 pos;	//位置
	XMFLOAT2 uv;	//UV座標
	XMFLOAT4 normal; //法線ベクトル
};

//前方宣言
class Texture;//Textureクラスってのがどこかにあってだなぁ宣言

class Quad
{
public:
	Quad();
	virtual ~Quad();
	virtual HRESULT Initialize();
	virtual void Draw(XMMATRIX& worldMatrix);
	void Release();
protected:
	ID3D11Buffer* pVertexBuffer_;	//頂点バッファ
	ID3D11Buffer* pIndexBuffer_;	//インデックスバッファ
	ID3D11Buffer* pConstantBuffer_;	//定数バッファ
	Texture* pTexture_;	//テクスチャ
};

