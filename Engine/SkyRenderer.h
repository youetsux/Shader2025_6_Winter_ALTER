#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

using namespace DirectX;

class SkyRenderer
{
private:
    ID3D11VertexShader*       pVertexShader_;
    ID3D11PixelShader*        pPixelShader_;
    ID3D11Buffer*             pConstantBuffer_;
    ID3D11ShaderResourceView* pCubemapSRV_;
    ID3D11SamplerState*       pSamplerState_;
    ID3D11DepthStencilState*  pDepthStencilState_;

    struct SKY_CONSTANT_BUFFER
    {
        XMMATRIX invProj;
        XMMATRIX invViewRot;
    };

public:
    SkyRenderer();
    ~SkyRenderer();

    HRESULT Initialize();
    void Draw();
    void Release();
};
