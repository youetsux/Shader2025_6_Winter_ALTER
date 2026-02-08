#pragma once

#include <d3d11.h>
#include <fbxsdk.h>
#include <string>
#include "Transform.h"
#include "Texture.h"
#include <vector>

#pragma comment(lib, "LibFbxSDK-MD.lib")
#pragma comment(lib, "LibXml2-MD.lib")
#pragma comment(lib, "zlib-MD.lib")

namespace Math
{
	//行列式を解く関数
	float Det(XMFLOAT3 a, XMFLOAT3 b, XMFLOAT3 c);
	//Rayと三角形の当たり判定を行う関数
	bool Intersect(XMFLOAT3 origin, XMFLOAT3 ray, XMFLOAT3 v0, XMFLOAT3 v1, XMFLOAT3 v2, float& dist);
}



//RayCastのためのデータを用意
struct RayCastData
{
	XMFLOAT4 start;//Rayの始点
	XMFLOAT4 dir;  //Rayの方向（正規化してあること）
	bool isHit;    //当たったかどうか
	float dist;    //始点からの距離
};

class Fbx
{
public:
	Fbx();
	HRESULT Load(std::string fileName);
	void    Draw(Transform& transform);
	void    Release();

	void InitVertex(FbxMesh* mesh);
	void InitIndex(FbxMesh* mesh);
	void InitConstantBuffer();
	void InitMaterial(FbxNode* pNode);
	void RayCast(RayCastData& rayData);
private:
	struct MATERIAL
	{
		Texture* pTexture;
		XMFLOAT4 diffuse;
		XMFLOAT4 ambient;
		XMFLOAT4 specular;
		float    shininess;
		XMFLOAT4 factor;
	};

	struct CONSTANT_BUFFER
	{
		XMMATRIX	matWVP; //ワールドビュー射影行列
		XMMATRIX	matWorld; //ワールド行列
		XMMATRIX	matNormal; //法線変換行列
		XMFLOAT4	diffuse; //材質の色
		XMFLOAT4    diffuseFactor; //拡散反射の強さ
		XMFLOAT4	specular; //鏡面反射の色
		XMFLOAT4	shininess; //鏡面反射の鋭さ 4要素同じのが入ってる
		XMFLOAT4	ambient; //環境光の色
		BOOL		materialFlag; //マテリアルがあるかないか
	};
	struct VERTEX
	{
		XMVECTOR position;
		XMVECTOR uv;
		XMVECTOR normal;
	};

	//バッファの皆さん
	ID3D11Buffer* pVertexBuffer_;
	ID3D11Buffer** pIndexBuffer_;
	ID3D11Buffer* pConstantBuffer_;
	std::vector<MATERIAL> pMaterialList_;
	std::vector<int> indexCount_;//マテリアルごとのインデックス数

	int vertexCount_;
	int polygonCount_;
	int materialCount_;

	//★　失われし古代のデータたち
	std::vector<VERTEX> pVertices_; //頂点データ全部
	std::vector<std::vector<int>> ppIndex_; //マテリアルごとのインデックスデータ [material][index]
	//auto& arr = ppIndex_[1];
	//arr[0]~arr[index - 1];

};

