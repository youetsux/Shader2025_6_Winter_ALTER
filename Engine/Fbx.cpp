#include "Fbx.h"
#include "Direct3D.h"
#include "Camera.h"
#include <filesystem>
#include <string>
#include <DirectXCollision.h>

namespace fs = std::filesystem;

Fbx::Fbx()
	: pVertexBuffer_(nullptr)
	, pIndexBuffer_(nullptr)
	, pConstantBuffer_(nullptr)
	, vertexCount_(0)
	, polygonCount_(0)
	, materialCount_(0)
	, pToonTexture_(nullptr)
{
}

HRESULT Fbx::Load(std::string fileName)
{
	using std::string;
	string subDir("Assets");
	fs::path currPath, basePath;
	currPath = fs::current_path();
	basePath = currPath;
	currPath = currPath / subDir;
	//fs::path subPath(currPath.string() + "\\" + subDir);
	assert(fs::exists(currPath));//subPathはあります、という確認
	fs::current_path(currPath);

	//マネージャを生成
	FbxManager* pFbxManager = FbxManager::Create();

	//インポーターを生成
	FbxImporter* fbxImporter = FbxImporter::Create(pFbxManager, "imp");
	fbxImporter->Initialize(fileName.c_str(), -1, pFbxManager->GetIOSettings());

	//シーンオブジェクトにFBXファイルの情報を流し込む
	FbxScene* pFbxScene = FbxScene::Create(pFbxManager, "fbxscene");
	fbxImporter->Import(pFbxScene);
	fbxImporter->Destroy();

	// ========== 変更1: 座標系の変換（右手系→左手系） ==========
	// 変換前の座標系を確認
	FbxAxisSystem sceneAxisSystem = pFbxScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem ourAxisSystem(FbxAxisSystem::DirectX);  // DirectX = Y-Up, Left-Handed
	
	if (sceneAxisSystem != ourAxisSystem)
	{
		// DeepConvertSceneで完全変換（頂点、法線、アニメーションすべて）
		ourAxisSystem.DeepConvertScene(pFbxScene);
		
		#ifdef _DEBUG
		OutputDebugStringA("座標系を DirectX (Y-Up, Left-Handed) に変換しました\n");
		#endif
	}
	// ========== 変更1 END ==========

	// ========== 変更2: 単位系の変換 ==========
	FbxSystemUnit sceneSystemUnit = pFbxScene->GetGlobalSettings().GetSystemUnit();
	if (sceneSystemUnit != FbxSystemUnit::cm)
	{
		// cmに変換
		FbxSystemUnit::cm.ConvertScene(pFbxScene);
		
		#ifdef _DEBUG
		OutputDebugStringA("単位系を cm に変換しました\n");
		#endif
	}
	// ========== 変更2 END ==========

	// ========== 変更3: 三角形化 ==========
	// FBXファイルには四角形ポリゴンが含まれる可能性があるため、事前に三角形化
	//FbxGeometryConverter geometryConverter(pFbxManager);
	//geometryConverter.Triangulate(pFbxScene, true);
	//
	//#ifdef _DEBUG
	//OutputDebugStringA("メッシュを三角形化しました\n");
	//#endif
	// ========== 変更3 END ==========

	//meshの情報を取得
	//メッシュ情報を取得
	FbxNode* rootNode = pFbxScene->GetRootNode();
	FbxNode* pNode = rootNode->GetChild(0);
	FbxMesh* mesh = pNode->GetMesh();

	//各情報の個数を取得

	vertexCount_ = mesh->GetControlPointsCount();	//頂点の数
	polygonCount_ = mesh->GetPolygonCount();	//ポリゴンの数
	materialCount_ = pNode->GetMaterialCount(); //マテリアルの数


	InitVertex(mesh);
	InitIndex(mesh);
	InitConstantBuffer();
	InitMaterial(pNode);

	fs::current_path(basePath);

	//マネージャ解放
	pFbxManager->Destroy();

	pToonTexture_ = new Texture();
	pToonTexture_->Load("Assets\\toon.png");

	return S_OK;
}

void Fbx::Draw(Transform& transform)
{
	Direct3D::SetShader(SHADER_3D);
	transform.Calculation();




	//for (int i = 0;i < materialCount_;i++)
	//{
	//	if (pMaterialList_[i].pTexture)
	//	{
	//		cb.materialFlag = { 1, 1, 1,1 };
	//		cb.diffuse = XMFLOAT4(1, 1, 1, 1);//保険
	//	}
	//	else
	//	{
	//		cb.materialFlag = { 0,0,0,0 };
	//		cb.diffuse = pMaterialList_[i].diffuse;
	//	}
	//}

	//頂点バッファ、インデックスバッファ、コンスタントバッファをパイプラインにセット
	//頂点バッファ
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	Direct3D::pContext->IASetVertexBuffers(0, 1, &pVertexBuffer_, &stride, &offset);

	for (int i = 0; i < materialCount_; i++)
	{
		CONSTANT_BUFFER cb;
		// ========== 変更4: XMMatrixTranspose()を削除（row_major指定のため不要） ==========
		// 変更前:
		// cb.matWVP = XMMatrixTranspose(transform.GetWorldMatrix() * Camera::GetViewMatrix() * Camera::GetProjectionMatrix());
		// cb.matWorld = XMMatrixTranspose(transform.GetWorldMatrix());
		// cb.matNormal = XMMatrixTranspose(transform.GetNormalMatrix());
		
		// 変更後: HLSLで row_major 指定している場合は転置不要
		cb.matWVP = transform.GetWorldMatrix() * Camera::GetViewMatrix() * Camera::GetProjectionMatrix();
		cb.matWorld = transform.GetWorldMatrix();
		cb.matNormal = transform.GetNormalMatrix();
		// ========== 変更4 END ==========
		
		cb.ambient = pMaterialList_[i].ambient;
		cb.specular = pMaterialList_[i].specular;
		cb.shininess = {pMaterialList_[i].shininess,
			pMaterialList_[i].shininess,
			pMaterialList_[i].shininess,
			pMaterialList_[i].shininess};
		cb.diffuse = pMaterialList_[i].diffuse;
		cb.diffuseFactor = pMaterialList_[i].factor;
		cb.materialFlag = pMaterialList_[i].pTexture != nullptr;




		D3D11_MAPPED_SUBRESOURCE pdata;
		Direct3D::pContext->Map(pConstantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &pdata);	// GPUからのデータアクセスを止める
		memcpy_s(pdata.pData, pdata.RowPitch, (void*)(&cb), sizeof(cb));	// データを値を送る

		Direct3D::pContext->Unmap(pConstantBuffer_, 0);	//再開

		// インデックスバッファーをセット
		stride = sizeof(int);
		offset = 0;
		Direct3D::pContext->IASetIndexBuffer(pIndexBuffer_[i], DXGI_FORMAT_R32_UINT, 0);

		//コンスタントバッファ
		Direct3D::pContext->VSSetConstantBuffers(0, 1, &pConstantBuffer_);	//頂点シェーダー用	
		Direct3D::pContext->PSSetConstantBuffers(0, 1, &pConstantBuffer_);	//ピクセルシェーダー用


		if (pMaterialList_[i].pTexture)
		{
			ID3D11SamplerState* pSampler = pMaterialList_[i].pTexture->GetSampler();
			Direct3D::pContext->PSSetSamplers(0, 1, &pSampler);

			ID3D11ShaderResourceView* pSRV = pMaterialList_[i].pTexture->GetSRV();
			Direct3D::pContext->PSSetShaderResources(0, 1, &pSRV);
		}

		//描画
		Direct3D::pContext->DrawIndexed(indexCount_[i], 0, 0);
	}
}

// ========== ノーマルマップ描画（新規） ==========
void Fbx::DrawNormalMapped(Transform& transform)
{
	Direct3D::SetShader(SHADER_NORMALMAP);  // ノーマルマップ用シェーダー
	transform.Calculation();

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	Direct3D::pContext->IASetVertexBuffers(0, 1, &pVertexBuffer_, &stride, &offset);

	for (int i = 0; i < materialCount_; i++)
	{
		CONSTANT_BUFFER cb;
		cb.matWVP = transform.GetWorldMatrix() * Camera::GetViewMatrix() * Camera::GetProjectionMatrix();
		cb.matWorld = transform.GetWorldMatrix();
		cb.matNormal = transform.GetNormalMatrix();

		cb.ambient = pMaterialList_[i].ambient;
		cb.specular = pMaterialList_[i].specular;
		cb.shininess = { pMaterialList_[i].shininess, pMaterialList_[i].shininess,
						pMaterialList_[i].shininess, pMaterialList_[i].shininess };
		cb.diffuse = pMaterialList_[i].diffuse;
		cb.diffuseFactor = pMaterialList_[i].factor;
		cb.materialFlag = pMaterialList_[i].pTexture != nullptr;

		D3D11_MAPPED_SUBRESOURCE pdata;
		Direct3D::pContext->Map(pConstantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &pdata);
		memcpy_s(pdata.pData, pdata.RowPitch, (void*)(&cb), sizeof(cb));
		Direct3D::pContext->Unmap(pConstantBuffer_, 0);

		stride = sizeof(int);
		offset = 0;
		Direct3D::pContext->IASetIndexBuffer(pIndexBuffer_[i], DXGI_FORMAT_R32_UINT, 0);

		Direct3D::pContext->VSSetConstantBuffers(0, 1, &pConstantBuffer_);
		Direct3D::pContext->PSSetConstantBuffers(0, 1, &pConstantBuffer_);

		// ========== Diffuse テクスチャのセット ==========
		if (pMaterialList_[i].pTexture)
		{
			ID3D11SamplerState* pSampler = pMaterialList_[i].pTexture->GetSampler();
			Direct3D::pContext->PSSetSamplers(0, 1, &pSampler);

			ID3D11ShaderResourceView* pSRV = pMaterialList_[i].pTexture->GetSRV();
			Direct3D::pContext->PSSetShaderResources(0, 1, &pSRV);
		}

		// ========== ノーマルマップのセット（ここが追加部分） ==========
		if (pMaterialList_[i].pNormalTexture)
		{
			ID3D11ShaderResourceView* pNormalSRV = pMaterialList_[i].pNormalTexture->GetSRV();
			Direct3D::pContext->PSSetShaderResources(1, 1, &pNormalSRV);  // スロット1にセット
		}
		// =============================================================

		Direct3D::pContext->DrawIndexed(indexCount_[i], 0, 0);
	}
}

void Fbx::DrawToon(Transform& transform)
{
	Direct3D::SetShader(SHADER_TOON);
	transform.Calculation();


	//頂点バッファ、インデックスバッファ、コンスタントバッファをパイプラインにセット
	//頂点バッファ
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	Direct3D::pContext->IASetVertexBuffers(0, 1, &pVertexBuffer_, &stride, &offset);

	for (int i = 0; i < materialCount_; i++)
	{
		CONSTANT_BUFFER cb;

		// 変更後: HLSLで row_major 指定している場合は転置不要
		cb.matWVP = transform.GetWorldMatrix() * Camera::GetViewMatrix() * Camera::GetProjectionMatrix();
		cb.matWorld = transform.GetWorldMatrix();
		cb.matNormal = transform.GetNormalMatrix();


		cb.ambient = pMaterialList_[i].ambient;
		cb.specular = pMaterialList_[i].specular;
		cb.shininess = { pMaterialList_[i].shininess,
			pMaterialList_[i].shininess,
			pMaterialList_[i].shininess,
			pMaterialList_[i].shininess };
		cb.diffuse = pMaterialList_[i].diffuse;
		cb.diffuseFactor = pMaterialList_[i].factor;
		cb.materialFlag = pMaterialList_[i].pTexture != nullptr;


		D3D11_MAPPED_SUBRESOURCE pdata;
		Direct3D::pContext->Map(pConstantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &pdata);	// GPUからのデータアクセスを止める
		memcpy_s(pdata.pData, pdata.RowPitch, (void*)(&cb), sizeof(cb));	// データを値を送る

		Direct3D::pContext->Unmap(pConstantBuffer_, 0);	//再開

		// インデックスバッファーをセット
		stride = sizeof(int);
		offset = 0;
		Direct3D::pContext->IASetIndexBuffer(pIndexBuffer_[i], DXGI_FORMAT_R32_UINT, 0);

		//コンスタントバッファ
		Direct3D::pContext->VSSetConstantBuffers(0, 1, &pConstantBuffer_);	//頂点シェーダー用	
		Direct3D::pContext->PSSetConstantBuffers(0, 1, &pConstantBuffer_);	//ピクセルシェーダー用


		if (pMaterialList_[i].pTexture)
		{
			ID3D11SamplerState* pSampler = pMaterialList_[i].pTexture->GetSampler();
			Direct3D::pContext->PSSetSamplers(0, 1, &pSampler);

			ID3D11ShaderResourceView* pSRV = pMaterialList_[i].pTexture->GetSRV();
			Direct3D::pContext->PSSetShaderResources(0, 1, &pSRV);
		}

		assert(pToonTexture_ != nullptr);
		if (pToonTexture_)
		{
			// トゥーンシェーダー用のテクスチャをセット
			ID3D11SamplerState* pSampler = pToonTexture_->GetSampler();
			Direct3D::pContext->PSSetSamplers(1, 1, &pSampler);

			ID3D11ShaderResourceView* pSRV = pToonTexture_->GetSRV();
			Direct3D::pContext->PSSetShaderResources(1, 1, &pSRV);
		}
		else
		{
			//エラー処理
		}

		//描画
		Direct3D::pContext->DrawIndexed(indexCount_[i], 0, 0);
	}
	Direct3D::SetShader(SHADER_3D);
}

void Fbx::Release()
{
}

void Fbx::InitVertex(FbxMesh* mesh)
{
	// ポリゴン数×3頂点に変更（各ポリゴンごとに独立した頂点を持つ）
	pVertices_.resize(polygonCount_ * 3);
	vertexCount_ = polygonCount_ * 3;  // 頂点数を更新

	int vertexIndex = 0;  // 展開後の頂点インデックス

	// Tangent の存在確認
	FbxLayerElementTangent* tangentElement = mesh->GetElementTangent();

	for (long poly = 0; poly < polygonCount_; poly++)
	{
		for (int vertex = 0; vertex < 3; vertex++)
		{
			// FBXの元の頂点インデックス
			int controlPointIndex = mesh->GetPolygonVertex(poly, vertex);

			// 頂点の位置
			FbxVector4 pos = mesh->GetControlPointAt(controlPointIndex);
			pVertices_[vertexIndex].position = XMVectorSet((float)pos[0], (float)pos[1], (float)pos[2], 0.0f);

			// 頂点のUV
			FbxLayerElementUV* pUV = mesh->GetLayer(0)->GetUVs();
			int uvIndex = mesh->GetTextureUVIndex(poly, vertex, FbxLayerElement::eTextureDiffuse);
			FbxVector2  uv = pUV->GetDirectArray().GetAt(uvIndex);
			pVertices_[vertexIndex].uv = XMVectorSet((float)uv.mData[0], (float)(1.0f - uv.mData[1]), 0.0f, 1.0f);

			// 頂点の法線
			FbxVector4 normal;
			mesh->GetPolygonVertexNormal(poly, vertex, normal);
			pVertices_[vertexIndex].normal = XMVectorSet((float)normal[0], (float)normal[1], (float)normal[2], 0.0f);

			//接線の取得（存在する場合）
					  // ========== Tangent（接線）の取得 ==========
			if (tangentElement != nullptr)
			{
				int tangentIndex = 0;

				// MappingMode に応じてインデックスを決定
				//if (tangentElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				//{
					// ポリゴン頂点ごとにデータがある場合
					if (tangentElement->GetReferenceMode() == FbxGeometryElement::eDirect)
					{
						tangentIndex = poly * 3 + vertex;
					}
					else if (tangentElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
					{
						tangentIndex = tangentElement->GetIndexArray().GetAt(poly * 3 + vertex);
					}
				//}
				//else if (tangentElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
				//{
				//	// コントロールポイントごとにデータがある場合
				//	if (tangentElement->GetReferenceMode() == FbxGeometryElement::eDirect)
				//	{
				//		tangentIndex = controlPointIndex;
				//	}
				//	else if (tangentElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
				//	{
				//		tangentIndex = tangentElement->GetIndexArray().GetAt(controlPointIndex);
				//	}
				//}

				FbxVector4 tangent = tangentElement->GetDirectArray().GetAt(tangentIndex);
				pVertices_[vertexIndex].tangent = XMVectorSet((float)tangent[0], (float)tangent[1], (float)tangent[2], 0.0f);
			}
			else
			{
				// Tangent が存在しない場合は仮の値（U方向）
				pVertices_[vertexIndex].tangent = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
			}

			//接線の取得ここまで


			vertexIndex++;  // 次の頂点へ
		}
	}

	// ========== Binormal（従法線）を計算で生成 ==========
	// すべての頂点に対して Binormal = Normal × Tangent を計算
	for (int i = 0; i < vertexCount_; i++)
	{
		XMVECTOR N = XMVector3Normalize(pVertices_[i].normal);
		XMVECTOR T = XMVector3Normalize(pVertices_[i].tangent);
		pVertices_[i].binormal = XMVector3Normalize(XMVector3Cross(N, T));
	}
	//for (int i = 0; i < polygonCount_; i++)
	//{
	//	int startIndex = mesh->GetPolygonVertexIndex(i);
	//	FbxVector4 tanget = mesh->GetElementTangent(0)->GetDirectArray().GetAt(startIndex);
	//	for (int j = 0; j < 3; j++)
	//	{
	//		int index = mesh->GetPolygonVertices()[startIndex + j];
	//		pVertices_[index].tangent = XMVectorSet((float)tanget[0], (float)tanget[1], (float)tanget[2], 0.0f);
	//	}
	//}

	// 頂点バッファ作成
	HRESULT hr;
	D3D11_BUFFER_DESC bd_vertex;
	bd_vertex.ByteWidth = sizeof(VERTEX) * vertexCount_;
	bd_vertex.Usage = D3D11_USAGE_DEFAULT;
	bd_vertex.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd_vertex.CPUAccessFlags = 0;
	bd_vertex.MiscFlags = 0;
	bd_vertex.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA data_vertex;
	data_vertex.pSysMem = pVertices_.data();
	hr = Direct3D::pDevice->CreateBuffer(&bd_vertex, &data_vertex, &pVertexBuffer_);
	if (FAILED(hr))
	{
		MessageBox(NULL, L"頂点バッファの作成に失敗しました", L"エラー", MB_OK);
	}
}

//void Fbx::InitVertex(FbxMesh* mesh)
//{
//	//VERTEX* vertices = new VERTEX[vertexCount_];
//	pVertices_.resize(vertexCount_); //修正
//	//全ポリゴン
//	for (long poly = 0; poly < polygonCount_; poly++)
//	{
//		//3頂点分
//		for (int vertex = 0; vertex < 3; vertex++)
//		{
//			//調べる頂点の番号
//			int index = mesh->GetPolygonVertex(poly, vertex);
//
//			//頂点の位置
//			FbxVector4 pos = mesh->GetControlPointAt(index);
//			//vertices[index].position 
//			pVertices_[index].position //修正
//				= XMVectorSet((float)pos[0], (float)pos[1], (float)pos[2], 0.0f);
//
//			//頂点のUV
//			FbxLayerElementUV* pUV = mesh->GetLayer(0)->GetUVs();
//			int uvIndex = mesh->GetTextureUVIndex(poly, vertex, FbxLayerElement::eTextureDiffuse);
//			FbxVector2  uv = pUV->GetDirectArray().GetAt(uvIndex);
//			//vertices[index].uv 
//			pVertices_[index].uv = XMVectorSet((float)uv.mData[0], (float)(1.0f - uv.mData[1]), 0.0f, 1.0f);
//		
//			//頂点の法線
//			FbxVector4 normal;
//			mesh->GetPolygonVertexNormal(poly, vertex, normal);
//			//vertices[index].normal
//			pVertices_[index].normal = XMVectorSet((float)normal[0], (float)normal[1], (float)normal[2], 0.0f);
//		}
//	}
//// 頂点バッファ作成
////（自分でやって）
////頂点バッファ
//	HRESULT hr;
//	D3D11_BUFFER_DESC bd_vertex;
//	bd_vertex.ByteWidth = sizeof(VERTEX) * vertexCount_;
//	bd_vertex.Usage = D3D11_USAGE_DEFAULT;
//	bd_vertex.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//	bd_vertex.CPUAccessFlags = 0;
//	bd_vertex.MiscFlags = 0;
//	bd_vertex.StructureByteStride = 0;
//	D3D11_SUBRESOURCE_DATA data_vertex;
//	//data_vertex.pSysMem = vertices;
//	data_vertex.pSysMem = pVertices_.data(); //修正
//	hr = Direct3D::pDevice->CreateBuffer(&bd_vertex, &data_vertex, &pVertexBuffer_);
//	if (FAILED(hr))
//	{
//		MessageBox(NULL, L"頂点バッファの作成に失敗しました", L"エラー", MB_OK);
//	}
//
//
//}

void Fbx::InitIndex(FbxMesh* mesh)
{
	pIndexBuffer_ = new ID3D11Buffer * [materialCount_];

	//int* index = new int[polygonCount_ * 3];
	ppIndex_.resize(materialCount_);
	indexCount_ = std::vector<int>(materialCount_);
	//indexCount_.resize(materialCount_);


	for (int i = 0; i < materialCount_; i++)
	{

		//int count = 0;
		auto& indeces = ppIndex_[i]; //修正

		//全ポリゴン
		for (long poly = 0; poly < polygonCount_; poly++)
		{

			FbxLayerElementMaterial* mtl = mesh->GetLayer(0)->GetMaterials();
			int mtlId = mtl->GetIndexArray().GetAt(poly);

			if (mtlId == i)
			{
				for (long vertex = 0; vertex < 3; vertex++)
				{
					//index[count] = mesh->GetPolygonVertex(poly, vertex);
					//indeces.push_back(mesh->GetPolygonVertex(poly, vertex)); //修正
					//count++;
					indeces.push_back((int)(poly * 3 + vertex)); //修正
				}
			}
		}

		//indexCount_[i] = count;
		indexCount_[i] = (int)indeces.size(); //修正


		//自力でどうぞ
		//	（ここもデータサイズを指定するところだけ注意）
		D3D11_BUFFER_DESC   bd;
		bd.Usage = D3D11_USAGE_DEFAULT;
		//bd.ByteWidth = sizeof(int) * polygonCount_ * 3;
		bd.ByteWidth = sizeof(int) * indexCount_[i];
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData;
		//InitData.pSysMem = index;
		InitData.pSysMem = indeces.data(); //修正
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		HRESULT hr;
		hr = Direct3D::pDevice->CreateBuffer(&bd, &InitData, &pIndexBuffer_[i]);
		if (FAILED(hr))
		{
			MessageBox(NULL, L"インデックスバッファの作成に失敗しました", L"エラー", MB_OK);
		}

	}

}

void Fbx::InitConstantBuffer()
{
	//Quadと一緒
	D3D11_BUFFER_DESC cb;
	cb.ByteWidth = sizeof(CONSTANT_BUFFER);
	cb.Usage = D3D11_USAGE_DYNAMIC;
	cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb.MiscFlags = 0;
	cb.StructureByteStride = 0;

	// コンスタントバッファの作成
	HRESULT hr;
	hr = Direct3D::pDevice->CreateBuffer(&cb, nullptr, &pConstantBuffer_);
	if (FAILED(hr))
	{
		MessageBox(NULL, L"コンスタントバッファの作成に失敗しました", L"エラー", MB_OK);
	}
}

void Fbx::InitMaterial(FbxNode* pNode)
{
    pMaterialList_.resize(materialCount_);
    for (int i = 0; i < materialCount_; i++)
    {
        FbxSurfaceMaterial* pMaterial = pNode->GetMaterial(i);
        
        // ========== Diffuse テクスチャの読み込み（既存） ==========
        FbxProperty diffuseProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
        int diffuseTextureCount = diffuseProperty.GetSrcObjectCount<FbxFileTexture>();
        
        if (diffuseTextureCount > 0)
        {
            FbxFileTexture* textureInfo = diffuseProperty.GetSrcObject<FbxFileTexture>(0);
            const char* textureFilePath = textureInfo->GetRelativeFileName();
            fs::path tPath(textureFilePath);
            
            if (fs::is_regular_file(tPath))
            {
                pMaterialList_[i].pTexture = new Texture;
                pMaterialList_[i].pTexture->Load(tPath.string());
            }
            else
            {
                pMaterialList_[i].pTexture = nullptr;
            }
        }
        else
        {
            pMaterialList_[i].pTexture = nullptr;
        }
        
        // ========== Normal Map（法線マップ）の読み込み ==========
        //FbxProperty normalMapProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sNormalMap);
        //int normalMapTextureCount = normalMapProperty.GetSrcObjectCount<FbxFileTexture>();
        //
        //if (normalMapTextureCount > 0)
        //{
        //    FbxFileTexture* normalMapInfo = normalMapProperty.GetSrcObject<FbxFileTexture>(0);
        //    const char* normalMapFilePath = normalMapInfo->GetRelativeFileName();
        //    fs::path nPath(normalMapFilePath);
        //    
        //    if (fs::is_regular_file(nPath))
        //    {
        //        pMaterialList_[i].pNormalTexture = new Texture;
        //        pMaterialList_[i].pNormalTexture->Load(nPath.string());
        //    }
        //    else
        //    {
        //        pMaterialList_[i].pNormalTexture = nullptr;
        //    }
        //}
        //else
        //{
        //    pMaterialList_[i].pNormalTexture = nullptr;
        //}
        
		fs::path defaultNormalPath = "textureNormal.png";

		if (fs::is_regular_file(defaultNormalPath))
		{
			pMaterialList_[i].pNormalTexture = new Texture;
			pMaterialList_[i].pNormalTexture->Load(defaultNormalPath.string());
		}
		else
		{
			pMaterialList_[i].pNormalTexture = nullptr;
		}

        // マテリアルパラメータの設定（既存のコード）
        FbxSurfacePhong* pPhong = (FbxSurfacePhong*)pNode->GetMaterial(i);
        if (pPhong)
        {
            FbxDouble diffuse = pPhong->DiffuseFactor;
            FbxDouble3 diffuseColor = pPhong->Diffuse;
            FbxDouble3 ambient = pPhong->Ambient;
            
            pMaterialList_[i].diffuse = XMFLOAT4((float)diffuseColor[0], (float)diffuseColor[1], (float)diffuseColor[2], 1.0f);
            pMaterialList_[i].factor = XMFLOAT4((float)diffuse, (float)diffuse, (float)diffuse, (float)diffuse);
            pMaterialList_[i].ambient = {(float)ambient[0], (float)ambient[1], (float)ambient[2], 1.0f};
            
            if (pPhong->GetClassId().Is(FbxSurfacePhong::ClassId))
            {
                FbxDouble3 specular = pPhong->Specular;
                FbxDouble shininess = pPhong->Shininess;
                pMaterialList_[i].specular = {(float)specular[0], (float)specular[1], (float)specular[2], 1.0f};
                pMaterialList_[i].shininess = shininess;
            }
        }
    }
}

void Fbx::RayCast(RayCastData& rayData)
{
	for (int material = 0; material < materialCount_; material++)
	{
		auto& indices = ppIndex_[material];
		//全ポリゴンに対して
		for (int i = 0; i < (int)indices.size(); i += 3)
		{
			VERTEX& V0 = pVertices_[indices[i + 0]];
			VERTEX& V1 = pVertices_[indices[i + 1]];
			VERTEX& V2 = pVertices_[indices[i + 2]];

			rayData.isHit = TriangleTests::Intersects(
								XMLoadFloat4(&rayData.start),
								XMLoadFloat4(&rayData.dir),
								V0.position,
								V1.position,
								V2.position,
								rayData.dist
								);
			//rayData.isHit = InterSects(V0, V1, V2, レイキャストのデータ);
			if (rayData.isHit)
			{
				return;
			}
		}
	}
	rayData.isHit = false;
}




////グループごとに全ポリゴンに対して
//   //頂点を3つとってくる
////XMVECTOR vv0 = pVertices_[ppIndex_[material][poly * 3 + 0]].position;
////XMVECTOR vv1 = pVertices_[ppIndex_[material][poly * 3 + 1]].position;
////XMVECTOR vv2 = pVertices_[ppIndex_[material][poly * 3 + 2]].position;
//XMVECTOR start = XMLoadFloat4(&rayData.start);
//XMVECTOR dir = XMLoadFloat4(&rayData.dir);
//XMVECTOR dirN = XMVector4Normalize(dir); //dirの単位ベクトル
////rayData.isHit = InterSects();