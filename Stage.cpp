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

// ========== スポットライトのパラメータ ==========
namespace
{
    XMFLOAT4 spotLightPos = { 0.0f, 2.0f, 0.0f, 1.0f };      // 上から照らす
    XMFLOAT4 spotLightDir = { 0.0f, -1.0f, 0.0f, 0.0f };     // 下向き
    float spotInnerAngle = XMConvertToRadians(15.0f);        // 内側の角度（明るい部分）
    float spotOuterAngle = XMConvertToRadians(30.0f);        // 外側の角度（減衰開始）
    
    // ========== オービットカメラのパラメータ ==========
    XMFLOAT3 cameraTarget = { 0.0f, 0.8f, 0.0f };           // 注視点
    float cameraDistance = 3.0f;                             // 注視点からの距離
    float cameraYaw = 0.0f;                                  // 水平角度（Y軸回転）
    float cameraPitch = 20.0f;                               // 垂直角度（上下）
}
// ===========================================================

Stage::Stage(GameObject* parent)
	:GameObject(parent, "Stage"),  pConstantBuffer_(nullptr)
{
	hball_ = -1;
	hRoom_ = -1;
	hGround_ = -1;
	hDonut_ = -1;
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
	
	// Skyの初期化
	HRESULT hr = sky_.Initialize();
	if (FAILED(hr))
	{
		MessageBox(NULL, L"Skyの初期化に失敗しました", L"エラー", MB_OK);
	}
	
	hball_ = Model::Load("ball.fbx");
	assert(hball_ >= 0);
	hRoom_ = Model::Load("room.fbx");
	assert(hRoom_ >= 0);
	hGround_ = Model::Load("plane3.fbx");
	assert(hGround_ >= 0);
	hDonut_ = Model::Load("normalmapedbox.fbx");
	assert(hDonut_ >= 0);
	//pMelbourne_ = new Sprite(L"Assets\\melbourne.png");
	
	// オービットカメラの初期設定
	cameraTarget = { 0.0f, 0.8f, 0.0f };
	cameraDistance = 3.0f;
	cameraYaw = 0.0f;
	cameraPitch = 20.0f;
}

void Stage::Update()
{
    transform_.rotate_.y += 0.5f;

    // ========== オービットカメラの更新 ==========
    // 球面座標からカメラ位置を計算
    float yawRad = XMConvertToRadians(cameraYaw);
    float pitchRad = XMConvertToRadians(cameraPitch);
    
    // 球面座標 → デカルト座標
    float x = cameraTarget.x + cameraDistance * cosf(pitchRad) * sinf(yawRad);
    float y = cameraTarget.y + cameraDistance * sinf(pitchRad);
    float z = cameraTarget.z + cameraDistance * cosf(pitchRad) * cosf(yawRad);
    
    Camera::SetPosition(XMVectorSet(x, y, z, 0));
    Camera::SetTarget(XMVectorSet(cameraTarget.x, cameraTarget.y, cameraTarget.z, 0));
    // ===========================================

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

    // ========== スポットライトの操作（新規） ==========
    // テンキーでスポットライトの位置を操作
    if (Input::IsKey(DIK_NUMPAD4)) spotLightPos.x -= 0.01f;
    if (Input::IsKey(DIK_NUMPAD6)) spotLightPos.x += 0.01f;
    if (Input::IsKey(DIK_NUMPAD8)) spotLightPos.z += 0.01f;
    if (Input::IsKey(DIK_NUMPAD2)) spotLightPos.z -= 0.01f;
    if (Input::IsKey(DIK_NUMPAD9)) spotLightPos.y += 0.01f;
    if (Input::IsKey(DIK_NUMPAD3)) spotLightPos.y -= 0.01f;
    // ==================================================

    // コンスタントバッファの設定と、シェーダーへのコンスタントバッファのセット
    CONSTANTBUFFER_STAGE cb;
    cb.lightPosition = Direct3D::GetLightPos();
    XMStoreFloat4(&cb.eyePosition, Camera::GetPosition());

    // ========== スポットライト情報を追加 ==========
    cb.spotLightPosition = spotLightPos;
    cb.spotLightDirection = spotLightDir;
    cb.spotLightParams = {
        cosf(spotInnerAngle),  // 内側の角度のコサイン
        cosf(spotOuterAngle),  // 外側の角度のコサイン
        0.0f,                  // 減衰係数（調整可能）
        0.0f                   // 未使用
    };
    // ==============================================

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
    // 最初に空を描画
    sky_.Draw();
    
    Transform ltr;
    ltr.position_ = { Direct3D::GetLightPos().x, Direct3D::GetLightPos().y, Direct3D::GetLightPos().z };
    ltr.scale_ = { 0.1, 0.1, 0.1 };
    Model::SetTransform(hball_, ltr);
    Model::Draw(hball_);

    //Transform tr;
    //tr.position_ = { 0, 0, 0 };
    //tr.rotate_ = { 0, 180, 0 };

    //Model::SetTransform(hRoom_, tr);
    //Model::Draw(hRoom_);



    Transform tr;
	tr.position_ = { 0, 0, 0 };
	tr.scale_ = { 5, 5, 5 };
	Model::SetTransform(hGround_, tr);
	Model::Draw(hGround_);


    static Transform tDonut;
    tDonut.scale_ = { 1, 1, 1 };
    tDonut.position_ = { 0, 0.5, 1.0 };
    tDonut.rotate_.y += 0.1;
    Model::SetTransform(hDonut_, tDonut);
    Model::DrawNormalMapped(hDonut_);

    // ========== ImGui カメラコントロール ==========
    ImGui::Begin("Camera Control");
    
    ImGui::Text("=== Orbit Camera ===");
    ImGui::SliderFloat("Yaw (Horizontal)", &cameraYaw, -180.0f, 180.0f);
    ImGui::SliderFloat("Pitch (Vertical)", &cameraPitch, -89.0f, 89.0f);
    ImGui::SliderFloat("Distance", &cameraDistance, 0.5f, 10.0f);
    
    ImGui::Separator();
    ImGui::DragFloat3("Target Position", &cameraTarget.x, 0.01f);
    
    if (ImGui::Button("Reset Camera"))
    {
        cameraTarget = { 0.0f, 0.8f, 0.0f };
        cameraDistance = 3.0f;
        cameraYaw = 0.0f;
        cameraPitch = 20.0f;
    }
    
    ImGui::End();
    // =============================================
}

void Stage::Release()
{
	sky_.Release();
}

























