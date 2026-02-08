#include "Model.h"
#include "Direct3D.h"

namespace Model
{
    std::vector<ModelData*> modelList;
}


int Model::Load(std::string fileName)
{
	ModelData* pModelData = new ModelData;
	pModelData->filename_ = fileName;
	pModelData->pfbx_ = nullptr;

	for (auto& itr : modelList)
	{
		if (itr->filename_ == fileName)
		{
			pModelData->pfbx_ = itr->pfbx_;
			break;
		}
	}

	if (pModelData->pfbx_ == nullptr)
	{
		pModelData->pfbx_ = new Fbx;
		pModelData->pfbx_->Load(fileName.c_str());
	}
	modelList.push_back(pModelData);
	return((int)(modelList.size() - 1));
	
}

void Model::SetTransform(int hModel, Transform transform)
{
	modelList[hModel]->transform_ = transform;
}

void Model::Draw(int hModel)
{
	modelList[hModel]->pfbx_
	                 ->Draw(modelList[hModel]->transform_);
}

void Model::Release()
{
	bool isReffered = false;//参照されているか
	for (int i=0;i<modelList.size();i++)
	{
		isReffered = false;
		for(int j = i + 1;j < modelList.size();j++)
		{
			if (modelList[i]->pfbx_ == modelList[j]->pfbx_)
			{
				isReffered = true;
				break;
			}
		}
		if (isReffered == false)
		{
			SAFE_DELETE(modelList[i]->pfbx_);
		}
		SAFE_DELETE(modelList[i]);
	}
	modelList.clear();//配列を空にする（念のため）
}

void Model::RayCast(int hModel, RayCastData& rayData)
{
	//⓪その時点での対象モデルのトランスフォームをカリキュレーション
	modelList[hModel]->transform_.Calculation();

	//ワールド行列取得
	XMMATRIX worldMatrix = modelList[hModel]->transform_.GetWorldMatrix();

	//①ワールド行列の逆行列
	XMMATRIX wInv = XMMatrixInverse(nullptr, worldMatrix);

	//②レイの通過点を求める(ワールド空間でのレイの始点からdir方向に進む直線上の点を計算）
	//方向ベクトルをちょい伸ばした先の点
	XMVECTOR vDirVec{ rayData.start.x + rayData.dir.x,
					  rayData.start.y + rayData.dir.y,
					  rayData.start.z + rayData.dir.z, 0.0f };

	//③rayData.startをモデル空間に変換（①をかける）
	XMVECTOR vstart = XMLoadFloat4(&rayData.start);

	//https://learn.microsoft.com/ja-jp/windows/win32/api/directxmath/
	//ここから、3次元ベクトルの変換関数を探す w=1のときの変換
	vstart = XMVector3Transform(vstart, wInv);
	XMStoreFloat4(&rayData.start, vstart); //変換結果をrayData.startに格納

	//④（始点から方向ベクトルをちょい伸ばした先）通過点（②）に①をかける(モデル空間に変換）
	vDirVec = XMVector3Transform(vDirVec, wInv);

	//⑤rayData.dirを③から④に向かうベクトルにする（位置と位置引き算＝ベクトル）
	XMVECTOR dirAtLocal = XMVectorSubtract(vDirVec, vstart);
	dirAtLocal = XMVector4Normalize(dirAtLocal); //正規化
	XMStoreFloat4(&rayData.dir, dirAtLocal); //変換結果をrayData.dirに格納
	

	//指定したモデル番号のFBXにレイキャスト！
	modelList[hModel]->pfbx_->RayCast(rayData);
}
