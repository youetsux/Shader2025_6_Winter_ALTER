#include "SkyRenderer.h"
#include "Direct3D.h"
#include "Camera.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <cassert>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

SkyRenderer::SkyRenderer()
    : pVertexShader_(nullptr)
    , pPixelShader_(nullptr)
    , pConstantBuffer_(nullptr)
    , pCubemapSRV_(nullptr)
    , pSamplerState_(nullptr)
    , pDepthStencilState_(nullptr)
{
}

SkyRenderer::~SkyRenderer()
{
    Release();
}

HRESULT SkyRenderer::Initialize()
{
    HRESULT hr;

    // ========== シェーダーのコンパイル ==========
    ID3DBlob* pCompileVS = nullptr;
    ID3DBlob* pCompilePS = nullptr;

    hr = D3DCompileFromFile(L"Sky.hlsl", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &pCompileVS, nullptr);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"Sky頂点シェーダーのコンパイルに失敗しました", L"エラー", MB_OK);
        return hr;
    }

    hr = D3DCompileFromFile(L"Sky.hlsl", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &pCompilePS, nullptr);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"Skyピクセルシェーダーのコンパイルに失敗しました", L"エラー", MB_OK);
        SAFE_RELEASE(pCompileVS);
        return hr;
    }

    hr = Direct3D::pDevice->CreateVertexShader(pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(), nullptr, &pVertexShader_);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"Sky頂点シェーダーの作成に失敗しました", L"エラー", MB_OK);
        SAFE_RELEASE(pCompileVS);
        SAFE_RELEASE(pCompilePS);
        return hr;
    }

    hr = Direct3D::pDevice->CreatePixelShader(pCompilePS->GetBufferPointer(), pCompilePS->GetBufferSize(), nullptr, &pPixelShader_);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"Skyピクセルシェーダーの作成に失敗しました", L"エラー", MB_OK);
        SAFE_RELEASE(pCompileVS);
        SAFE_RELEASE(pCompilePS);
        return hr;
    }

    SAFE_RELEASE(pCompileVS);
    SAFE_RELEASE(pCompilePS);

    // ========== 定数バッファの作成 ==========
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(SKY_CONSTANT_BUFFER);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

    hr = Direct3D::pDevice->CreateBuffer(&cbDesc, nullptr, &pConstantBuffer_);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"Sky定数バッファの作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // ========== キューブマップテクスチャの読み込み ==========
    const wchar_t* faceFiles[6] = {
        L"Assets\\px.png",  // +X (right)
        L"Assets\\nx.png",  // -X (left)
        L"Assets\\py.png",  // +Y (up)
        L"Assets\\ny.png",  // -Y (down)
        L"Assets\\pz.png",  // +Z (front)
        L"Assets\\nz.png"   // -Z (back)
    };

    // 各面の画像を読み込む
    TexMetadata metadata[6];
    ScratchImage images[6];

    for (int i = 0; i < 6; i++)
    {
        hr = LoadFromWICFile(faceFiles[i], WIC_FLAGS_NONE, &metadata[i], images[i]);
        if (FAILED(hr))
        {
            MessageBoxA(NULL, "キューブマップ画像の読み込みに失敗しました", "エラー", MB_OK);
            return hr;
        }
    }

    // キューブマップテクスチャを作成
    UINT width = static_cast<UINT>(metadata[0].width);
    UINT height = static_cast<UINT>(metadata[0].height);

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 6;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    ID3D11Texture2D* pCubemapTexture = nullptr;
    hr = Direct3D::pDevice->CreateTexture2D(&texDesc, nullptr, &pCubemapTexture);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"キューブマップテクスチャの作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // 各面のデータをコピー
    for (int i = 0; i < 6; i++)
    {
        const Image* img = images[i].GetImage(0, 0, 0);
        UINT subresource = D3D11CalcSubresource(0, i, 1);
        Direct3D::pContext->UpdateSubresource(pCubemapTexture, subresource, nullptr, img->pixels, static_cast<UINT>(img->rowPitch), 0);
    }

    // シェーダーリソースビューを作成
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.TextureCube.MostDetailedMip = 0;

    hr = Direct3D::pDevice->CreateShaderResourceView(pCubemapTexture, &srvDesc, &pCubemapSRV_);
    SAFE_RELEASE(pCubemapTexture);

    if (FAILED(hr))
    {
        MessageBox(NULL, L"キューブマップSRVの作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // ========== サンプラーステートの作成 ==========
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = Direct3D::pDevice->CreateSamplerState(&samplerDesc, &pSamplerState_);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"Skyサンプラーステートの作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    // ========== 深度ステンシルステートの作成 ==========
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // 深度書き込み無効
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    dsDesc.StencilEnable = FALSE;

    hr = Direct3D::pDevice->CreateDepthStencilState(&dsDesc, &pDepthStencilState_);
    if (FAILED(hr))
    {
        MessageBox(NULL, L"Sky深度ステンシルステートの作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    return S_OK;
}

void SkyRenderer::Draw()
{
    // ========== 定数バッファの更新 ==========
    XMMATRIX proj = Camera::GetProjectionMatrix();
    XMMATRIX view = Camera::GetViewMatrix();

    // 逆プロジェクション行列
    XMVECTOR det;
    XMMATRIX invProj = XMMatrixInverse(&det, proj);

    // ビュー行列から回転部分のみ抽出（平行移動を除去）
    XMMATRIX viewRot = view;
    viewRot.r[3] = XMVectorSet(0, 0, 0, 1);  // 平行移動成分を0に

    // 逆ビュー行列（回転のみ）
    XMMATRIX invViewRot = XMMatrixInverse(&det, viewRot);

    SKY_CONSTANT_BUFFER cb;
    cb.invProj = invProj;
    cb.invViewRot = invViewRot;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Direct3D::pContext->Map(pConstantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &cb, sizeof(SKY_CONSTANT_BUFFER));
        Direct3D::pContext->Unmap(pConstantBuffer_, 0);
    }

    // ========== 描画ステート設定 ==========
    // 深度ステートを変更（既存の深度ステートを保存）
    ID3D11DepthStencilState* pPrevDepthState = nullptr;
    UINT prevStencilRef = 0;
    Direct3D::pContext->OMGetDepthStencilState(&pPrevDepthState, &prevStencilRef);
    Direct3D::pContext->OMSetDepthStencilState(pDepthStencilState_, 0);

    // シェーダー設定
    Direct3D::pContext->VSSetShader(pVertexShader_, nullptr, 0);
    Direct3D::pContext->PSSetShader(pPixelShader_, nullptr, 0);

    // 入力レイアウトをNULLに（頂点バッファ不要）
    Direct3D::pContext->IASetInputLayout(nullptr);
    Direct3D::pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 定数バッファ設定
    Direct3D::pContext->VSSetConstantBuffers(0, 1, &pConstantBuffer_);

    // テクスチャとサンプラー設定
    Direct3D::pContext->PSSetShaderResources(0, 1, &pCubemapSRV_);
    Direct3D::pContext->PSSetSamplers(0, 1, &pSamplerState_);

    // 描画（頂点バッファなしで3頂点）
    Direct3D::pContext->Draw(3, 0);

    // ========== ステート復元 ==========
    Direct3D::pContext->OMSetDepthStencilState(pPrevDepthState, prevStencilRef);
    SAFE_RELEASE(pPrevDepthState);
}

void SkyRenderer::Release()
{
    SAFE_RELEASE(pDepthStencilState_);
    SAFE_RELEASE(pSamplerState_);
    SAFE_RELEASE(pCubemapSRV_);
    SAFE_RELEASE(pConstantBuffer_);
    SAFE_RELEASE(pPixelShader_);
    SAFE_RELEASE(pVertexShader_);
}
