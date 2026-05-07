#pragma once
#include "Engine\\GameObject.h"
#include <windows.h>
#include "Engine\\Sprite.h"

namespace
{
}
struct CONSTANTBUFFER_STAGE
{
	XMFLOAT4   lightPosition;  // 光源の位置 or 方向
	XMFLOAT4   eyePosition;    // カメラ位置
	int        lightType;      // 0=平行光源, 1=点光源
	XMFLOAT3   _pad;           // 16バイトアライメント用パディング
	XMFLOAT4X4 matLightVP;     // ライト視点の View×Projection 行列（影判定用）
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
	int lightType_;              // 0=平行光源, 1=点光源
	//Sprite* pMelbourne_;
	ID3D11Buffer* pConstantBuffer_;
	void InitConstantBuffer();
};

