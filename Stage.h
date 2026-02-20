#pragma once
#include "Engine\\GameObject.h"
#include "Engine\\SkyRenderer.h"
#include <windows.h>
#include "Engine\\Sprite.h"

namespace
{
}
struct CONSTANTBUFFER_STAGE
{
	XMFLOAT4 lightPosition;      // 点光源の位置
	XMFLOAT4 eyePosition;        // カメラ位置
	XMFLOAT4 spotLightPosition;  // ← スポットライトの位置
	XMFLOAT4 spotLightDirection; // ← スポットライトの方向
	XMFLOAT4 spotLightParams;    // ← x:内側角度cos, y:外側角度cos, z:減衰, w:未使用
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
	SkyRenderer sky_;
	void InitConstantBuffer();
};

