#include "Stage.h"
#include <string>
#include <vector>
#include "Engine//Model.h"
#include "resource.h"
#include <cassert>
#include "Engine/camera.h"
#include "Engine/Input.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"


Stage::Stage(GameObject* parent)
	:GameObject(parent, "Stage"),  pConstantBuffer_(nullptr)
{
	hball_ = -1;
	hRoom_ = -1;
	hGround_ = -1;
	hDonut_ = -1;
	lightType_ = 0;
}

Stage::~Stage()
{
}


void Stage::InitConstantBuffer()
{
	D3D11_BUFFER_DESC cb;
	cb.ByteWidth = sizeof(CONSTANTBUFFER_STAGE);
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

void Stage::Initialize()
{
	InitConstantBuffer();
	hball_ = Model::Load("ball.fbx");
	assert(hball_ >= 0);
	hRoom_ = Model::Load("room.fbx");
	assert(hRoom_ >= 0);
	hGround_ = Model::Load("plane3.fbx");
	assert(hGround_ >= 0);
	hDonut_ = Model::Load("normalmapedbox.fbx");
	assert(hDonut_ >= 0);
	//pMelbourne_ = new Sprite(L"Assets\\melbourne.png");
	Camera::SetPosition({ 0, 0.8, -2.8 });
	Camera::SetTarget({ 0,0.8,0 });

	// 比較サンプラーを作成して s1 にセット（第8章：影のエッジをなめらかにする）
	D3D11_SAMPLER_DESC sd = {};
	sd.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sd.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.BorderColor[0] = 1.0f;  // 範囲外は影なし（明るい）扱い
	sd.BorderColor[1] = 1.0f;
	sd.BorderColor[2] = 1.0f;
	sd.BorderColor[3] = 1.0f;
	sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	ID3D11SamplerState* pShadowSampler = nullptr;
	Direct3D::pDevice->CreateSamplerState(&sd, &pShadowSampler);
	Direct3D::pContext->PSSetSamplers(1, 1, &pShadowSampler);
	SAFE_RELEASE(pShadowSampler);
}

void Stage::Update()
{
    transform_.rotate_.y += 0.5f;

    // ========== 点光源の操作（既存） ==========
    if (Input::IsKey(DIK_A))
    {
        XMFLOAT4 p = Direct3D::GetLightPos();
        p = { p.x - 0.01f, p.y, p.z, p.w };
        Direct3D::SetLightPos(p);
    }
    if (Input::IsKey(DIK_D))
    {
        XMFLOAT4 p = Direct3D::GetLightPos();
        p = { p.x + 0.01f, p.y, p.z, p.w };
        Direct3D::SetLightPos(p);
    }
    if (Input::IsKey(DIK_W))
    {
        XMFLOAT4 p = Direct3D::GetLightPos();
        p = { p.x, p.y, p.z + 0.01f, p.w };
        Direct3D::SetLightPos(p);
    }
    if (Input::IsKey(DIK_S))
    {
        XMFLOAT4 p = Direct3D::GetLightPos();
        p = { p.x, p.y, p.z - 0.01f, p.w };
        Direct3D::SetLightPos(p);
    }
    if (Input::IsKey(DIK_UP))
    {
        XMFLOAT4 p = Direct3D::GetLightPos();
        p = { p.x, p.y + 0.01f, p.z, p.w };
        Direct3D::SetLightPos(p);
    }
    if (Input::IsKey(DIK_DOWN))
    {
        XMFLOAT4 p = Direct3D::GetLightPos();
        p = { p.x, p.y - 0.01f, p.z, p.w };
        Direct3D::SetLightPos(p);
    }

    // コンスタントバッファの設定
    CONSTANTBUFFER_STAGE cb;
    cb.lightPosition = Direct3D::GetLightPos();
    XMStoreFloat4(&cb.eyePosition, Camera::GetPosition());
    cb.lightType = lightType_;

    // ライト視点のVP行列を計算して送る（第7章で影判定に使う）
    XMMATRIX lightV  = Direct3D::GetLightViewMatrix();
    XMMATRIX lightP  = Direct3D::GetLightProjectionMatrix();
    XMMATRIX lightVP = lightV * lightP;
    XMStoreFloat4x4(&cb.matLightVP, lightVP);  // row_majorのため転置不要

    D3D11_MAPPED_SUBRESOURCE pdata;
    Direct3D::pContext->Map(pConstantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &pdata);
    memcpy_s(pdata.pData, pdata.RowPitch, (void*)(&cb), sizeof(cb));
    Direct3D::pContext->Unmap(pConstantBuffer_, 0);

    // コンスタントバッファ
    Direct3D::pContext->VSSetConstantBuffers(1, 1, &pConstantBuffer_);  // 頂点シェーダー用
    Direct3D::pContext->PSSetConstantBuffers(1, 1, &pConstantBuffer_);  // ピクセルシェーダー用
}

void Stage::Draw()
{
    // ===== トランスフォームの設定 =====
    Transform ltr;
    ltr.position_ = { Direct3D::GetLightPos().x, Direct3D::GetLightPos().y, Direct3D::GetLightPos().z };
    ltr.scale_ = { 0.1, 0.1, 0.1 };
    Model::SetTransform(hball_, ltr);

    Transform tr;
    tr.position_ = { 0, 0, 0 };
    tr.rotate_ = { 0, 180, 0 };
    Model::SetTransform(hRoom_, tr);

    static Transform tDonut;
    tDonut.scale_ = { 1, 1, 1 };
    tDonut.position_ = { 0, 0.5, 1.0 };
    tDonut.rotate_.y += 0.1;
    Model::SetTransform(hDonut_, tDonut);

    // ===== パス1：シャドウパス =====
    // ライト視点でドーナツの深度だけ書く
    Direct3D::BeginShadowPass();
    Model::DrawShadow(hDonut_);
    Direct3D::EndShadowPass();

    // ===== パス2：メインパス =====
    // シャドウマップを t1 にセットしてから描画する
    ID3D11ShaderResourceView* pShadowSRV = Direct3D::GetShadowMapSRV();
    Direct3D::pContext->PSSetShaderResources(1, 1, &pShadowSRV);

    // カメラ視点で普通に描画
    Model::Draw(hball_);
    Model::Draw(hRoom_);
    Model::Draw(hDonut_);

    // 描画後は必ず解除する（次フレームのDSVバインド競合を防ぐ）
    ID3D11ShaderResourceView* nullSRV = nullptr;
    Direct3D::pContext->PSSetShaderResources(1, 1, &nullSRV);

    // ========== ImGui ==========
    XMFLOAT4 lightPos = Direct3D::GetLightPos();
    ImGui::Text("Light: %.2f, %.2f, %.2f  [WASD+UD]", lightPos.x, lightPos.y, lightPos.z);
}

void Stage::Release()
{
}



