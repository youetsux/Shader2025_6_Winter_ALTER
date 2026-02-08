//#include "Texture.h"
//#include "Direct3D.h"
//#include <DirectXTex.h>
//
//// DirectXTexのライブラリをリンク
//#pragma comment(lib, "DirectXTex.lib")
//
//using namespace DirectX;
//
//Texture::Texture()
//{
//}
//
//Texture::~Texture()
//{
//}
//
//HRESULT Texture::Load(std::string fileName)
//{
//	TexMetadata metadata; //画像の付属情報
//
//	ScratchImage image;   //画像本体
//
//	HRESULT hr;
//
//	//実際に読んでゆくぅ　　　　　 
//	std::wstring wfileName(fileName.begin(), fileName.end());
//	hr = LoadFromWICFile(wfileName.c_str(), WIC_FLAGS::WIC_FLAGS_NONE,
//						 &metadata, image);
//	if (FAILED(hr))
//	{
//		return S_FALSE;
//	}
//
//	D3D11_SAMPLER_DESC  SamDesc;
//	ZeroMemory(&SamDesc, sizeof(D3D11_SAMPLER_DESC));
//	SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
//	SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
//	SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
//	SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
//	Direct3D::pDevice->CreateSamplerState(&SamDesc, &pSampler_);
//
//	//シェーダーリソースビュー
//
//	D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
//	srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
//	srv.Texture2D.MipLevels = 1;
//	hr = CreateShaderResourceView(Direct3D::pDevice,
//		image.GetImages(), image.GetImageCount(), metadata, &pSRV_);
//
//	return S_OK;
//}
//
//void Texture::Release()
//{
//	pSampler_->Release();
//	pSRV_->Release();
//}

#include "Texture.h"
#include "Direct3D.h"
#include <DirectXTex.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include <algorithm>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// DirectXTexライブラリをリンク
#pragma comment(lib, "DirectXTex.lib")

Texture::Texture()
    : pSRV_(nullptr)
    , pSampler_(nullptr)
{
}

Texture::~Texture()
{
    Release();
}

HRESULT Texture::Load(std::string fileName)
{
    // std::string → std::wstring 変換
    std::wstring wfileName(fileName.begin(), fileName.end());

    // 画像のメタデータと本体
    TexMetadata metadata;
    ScratchImage image;

    HRESULT hr;

    // ファイル拡張子から適切なローダーを選択
    size_t extPos = fileName.find_last_of('.');
    if (extPos == std::string::npos)
    {
        MessageBoxA(nullptr, "ファイル拡張子がありません", "Error", MB_OK);
        return E_FAIL;
    }

    std::string ext = fileName.substr(extPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // 拡張子に応じた読み込み
    if (ext == ".dds")
    {
        // DDSファイル（圧縮テクスチャ、キューブマップ対応）
        hr = LoadFromDDSFile(wfileName.c_str(), DDS_FLAGS_NONE, &metadata, image);
    }
    else if (ext == ".tga")
    {
        // TGAファイル
        hr = LoadFromTGAFile(wfileName.c_str(), TGA_FLAGS_NONE, &metadata, image);
    }
    else if (ext == ".hdr")
    {
        // HDRファイル（High Dynamic Range）
        hr = LoadFromHDRFile(wfileName.c_str(), &metadata, image);
    }
    else
    {
        // WIC対応フォーマット（PNG, JPEG, BMP, TIFF, GIF, ICO, WMP）
        hr = LoadFromWICFile(
            wfileName.c_str(),
            WIC_FLAGS_NONE,
            &metadata,
            image
        );
    }

    if (FAILED(hr))
    {
        std::string errorMsg = "テクスチャの読み込みに失敗: " + fileName;
        MessageBoxA(nullptr, errorMsg.c_str(), "Error", MB_OK);
        return hr;
    }

    // ミップマップ生成（元画像にミップマップがない場合）
    if (metadata.mipLevels == 1)
    {
        ScratchImage mipChain;
        hr = GenerateMipMaps(
            image.GetImages(),
            image.GetImageCount(),
            image.GetMetadata(),
            TEX_FILTER_DEFAULT,
            0,  // 0 = フルミップチェーン生成
            mipChain
        );

        if (SUCCEEDED(hr))
        {
            image = std::move(mipChain);
            metadata = image.GetMetadata();
        }
    }

    // シェーダーリソースビュー作成
    hr = CreateShaderResourceView(
        Direct3D::pDevice,
        image.GetImages(),
        image.GetImageCount(),
        metadata,
        &pSRV_
    );

    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "シェーダーリソースビューの作成に失敗", "Error", MB_OK);
        return hr;
    }

    // サンプラーステート作成（テクスチャごとに1つ）
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // トライリニアフィルタ
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // リピート
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;  // 全ミップレベル使用

    hr = Direct3D::pDevice->CreateSamplerState(&samplerDesc, &pSampler_);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "サンプラーステートの作成に失敗", "Error", MB_OK);
        return hr;
    }

    return S_OK;
}

void Texture::Release()
{
    // ComPtrなら自動解放されるが、念のため明示的にリセット
    if (pSampler_)
    {
        pSampler_->Release();
        pSampler_ = nullptr;
    }

    if (pSRV_)
    {
        pSRV_->Release();
        pSRV_ = nullptr;
    }
}