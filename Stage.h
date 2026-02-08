#pragma once
#include "Engine\\GameObject.h"
#include <windows.h>
#include "Engine\\Sprite.h"

namespace
{
}
struct CONSTANTBUFFER_STAGE
{
	XMFLOAT4 lightPosition;// 光源の位置(らんばーとのときは方向）
	XMFLOAT4 eyePosition; // カメラの位置
};

class Stage :
    public GameObject
{
public:
	Stage(GameObject *parent);
	~Stage();
	void Initialize() override;
	void Update() override;
	void Draw()override;
	void Release()override;
private:
	int hball_;    //モデル番号
	int hRoom_;
	int hGround_;
	int hDonut_;
	//Sprite* pMelbourne_;
	ID3D11Buffer* pConstantBuffer_;
	void InitConstantBuffer();
};

