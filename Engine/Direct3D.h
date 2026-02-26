#pragma once
#include <windows.h>
#include <d3d11.h>
#include <assert.h>
#include <DirectXMath.h>

//リンカ
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define SAFE_DELETE(p) if(p != nullptr){ delete p; p = nullptr;}
#define SAFE_RELEASE(p) if(p != nullptr){ p->Release(); p = nullptr;}

enum SHADER_TYPE
{
	SHADER_3D,	//3D用シェーダー
	SHADER_2D,	//2D用シェーダー
	SHADER_NORMALMAP, //法線マップ用シェーダー
	SHADER_TOON, //トゥーンシェーダー
	SHADER_MAX //シェーダーの最大数
};

namespace Direct3D
{
	//externはどこかに実際の定義（宣言）文あるぞって宣言
	extern ID3D11Device* pDevice;
	extern ID3D11DeviceContext* pContext;
	//シェーダー準備
	HRESULT InitShader(); //全てのシェーダー初期化
	//↑以下を初期化
	HRESULT InitShader3D();//3D用シェーダー初期化
	HRESULT InitShader2D();//2D用シェーダー初期化
	HRESULT InitNormalShader(); //法線マップ用シェーダー初期化
	HRESULT InitToonShader(); //トゥーンシェーダー初期化

	void SetShader(SHADER_TYPE type); //シェーダーをセット


	//初期化
	HRESULT Initialize(int winW, int winH, HWND hWnd);

	//描画開始
	void BeginDraw();

	//描画終了
	void EndDraw();

	//解放
	void Release();

	DirectX::XMFLOAT4 GetLightPos(); //ライトの位置
	void SetLightPos(DirectX::XMFLOAT4 pos); //ライトの位置設定
};