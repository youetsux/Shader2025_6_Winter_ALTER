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
	lightType_ = 0;  // デフォルト: 平行光源

	
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
	hDonut_ = Model::Load("Donut_phong.fbx");
    //hDonut_ = Model::Load("oden.fbx");
	assert(hDonut_ >= 0);
	//pMelbourne_ = new Sprite(L"Assets\\melbourne.png");
	Camera::SetPosition({ 0, 0.8, -2.8 });
	Camera::SetTarget({ 0,0.8,0 });
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



    // コンスタントバッファの設定と、シェーダーへのコンスタントバッファのセット
    CONSTANTBUFFER_STAGE cb;
    cb.lightPosition = Direct3D::GetLightPos();
    XMStoreFloat4(&cb.eyePosition, Camera::GetPosition());
    cb.lightType = lightType_;
    cb._pad = { 0,0,0 };


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
    Transform ltr;
    ltr.position_ = { Direct3D::GetLightPos().x, Direct3D::GetLightPos().y, Direct3D::GetLightPos().z };
    ltr.scale_ = { 0.1, 0.1, 0.1 };
    Model::SetTransform(hball_, ltr);
    Model::Draw(hball_);

    Transform tr;
    tr.position_ = { 0, 0, 0 };
    tr.rotate_ = { 0, 180, 0 };

    Model::SetTransform(hRoom_, tr);
    Model::Draw(hRoom_);

    static Transform tDonut;
    tDonut.scale_ = { 0.2, 0.2, 0.2 };
    tDonut.position_ = { 0, 0.5, 0.0 };
    tDonut.rotate_.y += 0.1;
    Model::SetTransform(hDonut_, tDonut);
    Model::Draw(hDonut_);

    // ========== ImGui でライト情報を表示 =========
    ImGui::Text("Stage Class rot: %lf", tDonut.rotate_.z);

    ImGui::Separator();
    ImGui::Text("=== Light Type ===");
    if (ImGui::Button("Directional")) { lightType_ = 0; }
    ImGui::SameLine();
    if (ImGui::Button("Point")) { lightType_ = 1; }
    ImGui::SameLine();
    ImGui::Text("Current: %s", lightType_ == 0 ? "Directional" : "Point");

    ImGui::Separator();
    ImGui::Text("=== Light Information ===");

    XMFLOAT4 pointLight = Direct3D::GetLightPos();
    if (lightType_ == 1)
    {
        ImGui::Text("Point Light Position:");
        ImGui::Text("  X: %.2f, Y: %.2f, Z: %.2f", pointLight.x, pointLight.y, pointLight.z);
        ImGui::Text("  Control: WASD + Up/Down");
    }
    else
    {
        ImGui::Text("Directional Light Direction:");
        ImGui::Text("  X: %.2f, Y: %.2f, Z: %.2f", pointLight.x, pointLight.y, pointLight.z);
        ImGui::Text("  Control: WASD + Up/Down");
    }

    ImGui::Separator();

    // ========== Step1 デバッグ：ライト行列を表示 ==========
    if (ImGui::CollapsingHeader("Light Matrix Debug"))
    {
        XMMATRIX V  = Direct3D::GetLightViewMatrix();
        XMMATRIX P  = Direct3D::GetLightProjectionMatrix();
        XMMATRIX VP = V * P;

        ImGui::Text("-- LightView --");
        ImGui::Text("[0]: %.2f %.2f %.2f %.2f", V.r[0].m128_f32[0], V.r[0].m128_f32[1], V.r[0].m128_f32[2], V.r[0].m128_f32[3]);
        ImGui::Text("[1]: %.2f %.2f %.2f %.2f", V.r[1].m128_f32[0], V.r[1].m128_f32[1], V.r[1].m128_f32[2], V.r[1].m128_f32[3]);
        ImGui::Text("[2]: %.2f %.2f %.2f %.2f", V.r[2].m128_f32[0], V.r[2].m128_f32[1], V.r[2].m128_f32[2], V.r[2].m128_f32[3]);
        ImGui::Text("[3]: %.2f %.2f %.2f %.2f", V.r[3].m128_f32[0], V.r[3].m128_f32[1], V.r[3].m128_f32[2], V.r[3].m128_f32[3]);
    }
    // ========== Step1 デバッグ END ==========

    // ========== Step2 デバッグ：シャドウマップの生成確認 ==========
    ImGui::Text("ShadowMap SRV: %s",
        Direct3D::GetShadowMapSRV() != nullptr ? "OK" : "null");
    // ===============================================================

}
void Stage::Release()
{
}



