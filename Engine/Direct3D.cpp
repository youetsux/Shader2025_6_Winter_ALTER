#include "Direct3D.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"



using namespace DirectX;

namespace Direct3D
{
	ID3D11Device* pDevice;		                    //デバイス
	ID3D11DeviceContext* pContext;	                //デバイスコンテキスト
	IDXGISwapChain* pSwapChain;		                //スワップチェイン
	ID3D11RenderTargetView* pRenderTargetView;	    //レンダーターゲットビュー
    ID3D11Texture2D* pDepthStencil;			//深度ステンシル
    ID3D11DepthStencilView* pDepthStencilView;		//深度ステンシルビュー


	struct SHADER_BUNDLE
	{
		ID3D11VertexShader* pVertexShader;	//頂点シェーダー
		ID3D11PixelShader* pPixelShader;		//ピクセルシェーダー
		ID3D11InputLayout* pVertexLayout;	//頂点インプットレイアウト
		ID3D11RasterizerState* pRasterizerState;	//ラスタライザー
	};
    
    ID3D11VertexShader* pVertexShader = nullptr;	//頂点シェーダー
    ID3D11PixelShader* pPixelShader = nullptr;		//ピクセルシェーダー

    ID3D11InputLayout* pVertexLayout = nullptr;	//頂点インプットレイアウト
    ID3D11RasterizerState* pRasterizerState = nullptr;	//ラスタライザー
	
	SHADER_BUNDLE shaderBundle[SHADER_MAX];	//シェーダーのバンドル
	XMFLOAT4 lightPosition{ 0.5f, -1.0f, 0.7f, 0.0f }; // 平行光源の方向ベクトル (右斜め前上から照らす)

	int screenWidth  = 0; // 画面幅（EndShadowPass でビューポートを戻すために保存）
	int screenHeight = 0; // 画面高さ

	// ========== シャドウマップ用リソース ==========
	ID3D11Texture2D*          pShadowMapTexture = nullptr; // 深度を格納するテクスチャ
	ID3D11DepthStencilView*   pShadowMapDSV     = nullptr; // 書き込み口
	ID3D11ShaderResourceView* pShadowMapSRV     = nullptr; // 読み込み口
	// ========== シャドウマップ用リソース END ==========
}


HRESULT Direct3D::InitShader()
{
	if (FAILED(InitShader3D()))
	{
		return E_FAIL;
	}
    if (FAILED(InitShader2D()))
    {
        return E_FAIL;
    }
    if (FAILED(InitNormalShader()))
    {
        return E_FAIL;
    }
	if(FAILED(InitToonShader()))
	{
		return E_FAIL;
	}
	if (FAILED(InitShadowShader()))
	{
		return E_FAIL;
	}
	return S_OK;
}

//NormalShader.hlsl用のシェーダーの初期化
//プロトタイプまだ書いてねぇわ
HRESULT Direct3D::InitNormalShader()
{
    HRESULT hr;


    // 頂点シェーダの作成（コンパイル）
    ID3DBlob* pCompileVS = nullptr;
    D3DCompileFromFile(L"NormalShader.hlsl", nullptr, nullptr, "VS", "vs_5_0", NULL, 0, &pCompileVS, NULL);
    assert(pCompileVS != nullptr);


    hr = pDevice->CreateVertexShader(pCompileVS->GetBufferPointer(), 
        pCompileVS->GetBufferSize(), NULL, &(shaderBundle[SHADER_NORMALMAP].pVertexShader));
    
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"頂点シェーダの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }



    // ピクセルシェーダの作成（コンパイル）
    ID3DBlob* pCompilePS = nullptr;
    D3DCompileFromFile(L"NormalShader.hlsl", nullptr, nullptr, "PS", "ps_5_0", NULL, 0, &pCompilePS, NULL);
    assert(pCompilePS != nullptr);
    hr = pDevice->CreatePixelShader(pCompilePS->GetBufferPointer(),
        pCompilePS->GetBufferSize(), NULL, &(shaderBundle[SHADER_NORMALMAP].pPixelShader));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"ピクセルシェーダの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    UINT offset[5] = {0, sizeof(DirectX::XMVECTOR), sizeof(DirectX::XMVECTOR) * 2, sizeof(DirectX::XMVECTOR) * 3, sizeof(DirectX::XMVECTOR) * 4};
    //オフセット計算:
    //    -POSITION  :  0 bytes(4 floats = 16 bytes)
    //    - TEXCOORD : 16 bytes(4 floats = 16 bytes)
    //    - NORMAL   : 32 bytes(4 floats = 16 bytes)
    //    - TANGENT  : 48 bytes(4 floats = 16 bytes) ← 追加
    //    - BINORMAL : 64 bytes(4 floats = 16 bytes) ← 追加
    //          合計 : 80 bytes

    //頂点インプットレイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  offset[0],  D3D11_INPUT_PER_VERTEX_DATA, 0 },	//位置
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0,  offset[1],  D3D11_INPUT_PER_VERTEX_DATA, 0 },//UV座標
	    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  offset[2],  D3D11_INPUT_PER_VERTEX_DATA, 0 }, //法線ベクトル
	    { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  offset[3],  D3D11_INPUT_PER_VERTEX_DATA, 0 }, //接線ベクトル
        {"BINORMAL",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  offset[4],  D3D11_INPUT_PER_VERTEX_DATA, 0 } //従法線ベクトル
    };

    //パラメータ数間違えてた
    hr = pDevice->CreateInputLayout(layout, 5, pCompileVS->GetBufferPointer(),
        pCompileVS->GetBufferSize(), &(shaderBundle[SHADER_NORMALMAP].pVertexLayout));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"頂点インプットレイアウトの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }


    pCompileVS->Release();
    pCompilePS->Release();
    //ラスタライザ作成
    D3D11_RASTERIZER_DESC rdc = {};
    rdc.CullMode = D3D11_CULL_BACK;
    rdc.FillMode = D3D11_FILL_SOLID;
    rdc.FrontCounterClockwise = FALSE;
    pDevice->CreateRasterizerState(&rdc, &(shaderBundle[SHADER_NORMALMAP].pRasterizerState));

    //それぞれをデバイスコンテキストにセット
    //pContext->VSSetShader(pVertexShader, NULL, 0);	//頂点シェーダー
    //pContext->PSSetShader(pPixelShader, NULL, 0);	//ピクセルシェーダー
    //pContext->IASetInputLayout(pVertexLayout);	//頂点インプットレイアウト
    //pContext->RSSetState(pRasterizerState);		//ラスタライザー


    return S_OK;
}


HRESULT Direct3D::InitToonShader()

{
    HRESULT hr;


    // 頂点シェーダの作成（コンパイル）
    ID3DBlob* pCompileVS = nullptr;
    D3DCompileFromFile(L"toonShader.hlsl", nullptr, nullptr, "VS", "vs_5_0", NULL, 0, &pCompileVS, NULL);
    assert(pCompileVS != nullptr);


    hr = pDevice->CreateVertexShader(pCompileVS->GetBufferPointer(),
        pCompileVS->GetBufferSize(), NULL, &(shaderBundle[SHADER_TOON].pVertexShader));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"頂点シェーダの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }



    // ピクセルシェーダの作成（コンパイル）
    ID3DBlob* pCompilePS = nullptr;
    D3DCompileFromFile(L"toonShader.hlsl", nullptr, nullptr, "PS", "ps_5_0", NULL, 0, &pCompilePS, NULL);
    assert(pCompilePS != nullptr);
    hr = pDevice->CreatePixelShader(pCompilePS->GetBufferPointer(),
        pCompilePS->GetBufferSize(), NULL, &(shaderBundle[SHADER_TOON].pPixelShader));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"ピクセルシェーダの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    UINT offset[] = { 0, sizeof(DirectX::XMVECTOR), sizeof(DirectX::XMVECTOR) * 2 };
    //オフセット計算:
    //    -POSITION  :  0 bytes(4 floats = 16 bytes)
    //    - TEXCOORD : 16 bytes(4 floats = 16 bytes)
    //    - NORMAL   : 32 bytes(4 floats = 16 bytes)
    //          合計 : 48 bytes

    //頂点インプットレイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  offset[0],  D3D11_INPUT_PER_VERTEX_DATA, 0 },	//位置
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0,  offset[1],  D3D11_INPUT_PER_VERTEX_DATA, 0 },//UV座標
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  offset[2],  D3D11_INPUT_PER_VERTEX_DATA, 0 }, //法線ベクトル
    };

    hr = pDevice->CreateInputLayout(layout, 3, pCompileVS->GetBufferPointer(),
        pCompileVS->GetBufferSize(), &(shaderBundle[SHADER_TOON].pVertexLayout));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"頂点インプットレイアウトの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }


    pCompileVS->Release();
    pCompilePS->Release();
    //ラスタライザ作成
    D3D11_RASTERIZER_DESC rdc = {};
    rdc.CullMode = D3D11_CULL_BACK;
    rdc.FillMode = D3D11_FILL_SOLID;
    rdc.FrontCounterClockwise = FALSE;
    pDevice->CreateRasterizerState(&rdc, &(shaderBundle[SHADER_TOON].pRasterizerState));

    //それぞれをデバイスコンテキストにセット
    //pContext->VSSetShader(pVertexShader, NULL, 0);	//頂点シェーダー
    //pContext->PSSetShader(pPixelShader, NULL, 0);	//ピクセルシェーダー
    //pContext->IASetInputLayout(pVertexLayout);	//頂点インプットレイアウト
    //pContext->RSSetState(pRasterizerState);		//ラスタライザー


    return S_OK;
}

HRESULT Direct3D::InitShader3D()
{
    HRESULT hr;


    // 頂点シェーダの作成（コンパイル）
    ID3DBlob* pCompileVS = nullptr;
    D3DCompileFromFile(L"Simple3D.hlsl", nullptr, nullptr, "VS", "vs_5_0", NULL, 0, &pCompileVS, NULL);
    assert(pCompileVS != nullptr);


    hr = pDevice->CreateVertexShader(pCompileVS->GetBufferPointer(),
        pCompileVS->GetBufferSize(), NULL, &(shaderBundle[SHADER_3D].pVertexShader));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"頂点シェーダの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }



    // ピクセルシェーダの作成（コンパイル）
    ID3DBlob* pCompilePS = nullptr;
    D3DCompileFromFile(L"Simple3D.hlsl", nullptr, nullptr, "PS", "ps_5_0", NULL, 0, &pCompilePS, NULL);
    assert(pCompilePS != nullptr);
    hr = pDevice->CreatePixelShader(pCompilePS->GetBufferPointer(),
        pCompilePS->GetBufferSize(), NULL, &(shaderBundle[SHADER_3D].pPixelShader));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"ピクセルシェーダの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    UINT offset[] = { 0, sizeof(DirectX::XMVECTOR), sizeof(DirectX::XMVECTOR) * 2};
    //オフセット計算:
    //    -POSITION  :  0 bytes(4 floats = 16 bytes)
    //    - TEXCOORD : 16 bytes(4 floats = 16 bytes)
    //    - NORMAL   : 32 bytes(4 floats = 16 bytes)
    //          合計 : 48 bytes

    //頂点インプットレイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  offset[0],  D3D11_INPUT_PER_VERTEX_DATA, 0 },	//位置
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0,  offset[1],  D3D11_INPUT_PER_VERTEX_DATA, 0 },//UV座標
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  offset[2],  D3D11_INPUT_PER_VERTEX_DATA, 0 }, //法線ベクトル
    };

    hr = pDevice->CreateInputLayout(layout, 3, pCompileVS->GetBufferPointer(),
        pCompileVS->GetBufferSize(), &(shaderBundle[SHADER_3D].pVertexLayout));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"頂点インプットレイアウトの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }


    pCompileVS->Release();
    pCompilePS->Release();
    //ラスタライザ作成
    D3D11_RASTERIZER_DESC rdc = {};
    rdc.CullMode = D3D11_CULL_BACK;
    rdc.FillMode = D3D11_FILL_SOLID;
    rdc.FrontCounterClockwise = FALSE;
    pDevice->CreateRasterizerState(&rdc, &(shaderBundle[SHADER_3D].pRasterizerState));

    //それぞれをデバイスコンテキストにセット
    //pContext->VSSetShader(pVertexShader, NULL, 0);	//頂点シェーダー
    //pContext->PSSetShader(pPixelShader, NULL, 0);	//ピクセルシェーダー
    //pContext->IASetInputLayout(pVertexLayout);	//頂点インプットレイアウト
    //pContext->RSSetState(pRasterizerState);		//ラスタライザー


    return S_OK;
}

HRESULT Direct3D::InitShader2D()
{
    HRESULT hr;


    // 頂点シェーダの作成（コンパイル）
    ID3DBlob* pCompileVS = nullptr;
    D3DCompileFromFile(L"Simple2D.hlsl", nullptr, nullptr, "VS", "vs_5_0", NULL, 0, &pCompileVS, NULL);
    assert(pCompileVS != nullptr);


    hr = pDevice->CreateVertexShader(pCompileVS->GetBufferPointer(), 
        pCompileVS->GetBufferSize(), NULL, &(shaderBundle[SHADER_2D].pVertexShader));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"頂点シェーダの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }



    // ピクセルシェーダの作成（コンパイル）
    ID3DBlob* pCompilePS = nullptr;
    D3DCompileFromFile(L"Simple2D.hlsl", nullptr, nullptr, "PS", "ps_5_0", NULL, 0, &pCompilePS, NULL);
    assert(pCompilePS != nullptr);
    hr = pDevice->CreatePixelShader(pCompilePS->GetBufferPointer(), 
        pCompilePS->GetBufferSize(), NULL, &(shaderBundle[SHADER_2D].pPixelShader));
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"ピクセルシェーダの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }

    //頂点インプットレイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },	//位置
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  sizeof(DirectX::XMVECTOR), D3D11_INPUT_PER_VERTEX_DATA, 0 },//UV座標
    };

    hr = pDevice->CreateInputLayout(layout, 2, pCompileVS->GetBufferPointer(),
        pCompileVS->GetBufferSize(), &(shaderBundle[SHADER_2D].pVertexLayout));

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"頂点インプットレイアウトの作成の作成に失敗しました", L"エラー", MB_OK);
        return hr;
    }


    pCompileVS->Release();
    pCompilePS->Release();
    //ラスタライザ作成
    D3D11_RASTERIZER_DESC rdc = {};
    rdc.CullMode = D3D11_CULL_NONE;
    rdc.FillMode = D3D11_FILL_SOLID;
    rdc.FrontCounterClockwise = FALSE;
    pDevice->CreateRasterizerState(&rdc, &(shaderBundle[SHADER_2D].pRasterizerState));

    //それぞれをデバイスコンテキストにセット
    //pContext->VSSetShader(pVertexShader, NULL, 0);	//頂点シェーダー
    //pContext->PSSetShader(pPixelShader, NULL, 0);	//ピクセルシェーダー
    //pContext->IASetInputLayout(pVertexLayout);	//頂点インプットレイアウト
    //pContext->RSSetState(pRasterizerState);		//ラスタライザー


    return S_OK;
}

void Direct3D::SetShader(SHADER_TYPE type)
{
    pContext->VSSetShader(shaderBundle[type].pVertexShader, NULL, 0);	//頂点シェーダー
    pContext->PSSetShader(shaderBundle[type].pPixelShader, NULL, 0);	//ピクセルシェーダー
    pContext->IASetInputLayout(shaderBundle[type].pVertexLayout);	//頂点インプットレイアウト
    pContext->RSSetState(shaderBundle[type].pRasterizerState);		//ラスタライザー
}

HRESULT Direct3D::Initialize(int winW, int winH, HWND hWnd)
{
    screenWidth  = winW;
    screenHeight = winH;

    // Direct3Dの初期化
    DXGI_SWAP_CHAIN_DESC scDesc = {};
    //とりあえず全部0
    ZeroMemory(&scDesc, sizeof(scDesc));
    //描画先のフォーマット
    scDesc.BufferDesc.Width = winW;		//画面幅
    scDesc.BufferDesc.Height = winH;	//画面高さ
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// 何色使えるか

    //FPS（1/60秒に1回）
    scDesc.BufferDesc.RefreshRate.Numerator = 60;
    scDesc.BufferDesc.RefreshRate.Denominator = 1;

    //その他
    scDesc.Windowed = TRUE;			//ウィンドウモードかフルスクリーンか
    scDesc.OutputWindow = hWnd;		//ウィンドウハンドル
    scDesc.BufferCount = 1;			//バックバッファの枚数
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//バックバッファの使い道＝画面に描画するために
    scDesc.SampleDesc.Count = 1;		//MSAA（アンチエイリアス）の設定
    scDesc.SampleDesc.Quality = 0;		//　〃

    ////////////////上記設定をもとにデバイス、コンテキスト、スワップチェインを作成////////////////////////
    D3D_FEATURE_LEVEL level;
    D3D11CreateDeviceAndSwapChain(
        nullptr,				// どのビデオアダプタを使用するか？既定ならばnullptrで
        D3D_DRIVER_TYPE_HARDWARE,		// ドライバのタイプを渡す。ふつうはHARDWARE
        nullptr,				// 上記をD3D_DRIVER_TYPE_SOFTWAREに設定しないかぎりnullptr
        0,					// 何らかのフラグを指定する。（デバッグ時はD3D11_CREATE_DEVICE_DEBUG？）
        nullptr,				// デバイス、コンテキストのレベルを設定。nullptrにしとけばOK
        0,					// 上の引数でレベルを何個指定したか
        D3D11_SDK_VERSION,			// SDKのバージョン。必ずこの値
        &scDesc,				// 上でいろいろ設定した構造体
        &pSwapChain,				// 無事完成したSwapChainのアドレスが返ってくる
        &pDevice,				// 無事完成したDeviceアドレスが返ってくる
        &level,					// 無事完成したDevice、Contextのレベルが返ってくる
        &pContext);				// 無事完成したContextのアドレスが返ってくる

    ///////////////////////////レンダーターゲットビュー作成///////////////////////////////
    //スワップチェーンからバックバッファを取得（バックバッファ ＝ レンダーターゲット）
    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    //レンダーターゲットビューを作成
    pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);

    //一時的にバックバッファを取得しただけなので解放
    pBackBuffer->Release();

    ///////////////////////////ビューポート（描画範囲）設定///////////////////////////////
//レンダリング結果を表示する範囲
    D3D11_VIEWPORT vp;
    vp.Width = (float)winW;	//幅
    vp.Height = (float)winH;//高さ
    vp.MinDepth = 0.0f;	//手前
    vp.MaxDepth = 1.0f;	//奥
    vp.TopLeftX = 0;	//左
    vp.TopLeftY = 0;	//上
	///////////////////////////深度ステンシルビュー作成///////////////////////////////
        //深度ステンシルビューの作成
    D3D11_TEXTURE2D_DESC descDepth;
    descDepth.Width = winW;
    descDepth.Height = winH;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D32_FLOAT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    pDevice->CreateTexture2D(&descDepth, NULL, &pDepthStencil);
    pDevice->CreateDepthStencilView(pDepthStencil, NULL, &pDepthStencilView);




    //データを画面に描画するための一通りの設定（パイプライン）
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  // データの入力種類を指定
    pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);            // 描画先を設定
    pContext->RSSetViewports(1, &vp);

    HRESULT hr;
    hr = InitShader();
    if (FAILED(hr))
    {
        return hr;
    }

    // ========== シャドウマップ初期化 ==========
    hr = InitShadowMap(1024, 1024); // 解像度 1024x1024
    if (FAILED(hr)) { return hr; }
    // ==========================================

    return S_OK;

}

void Direct3D::BeginDraw()
{
    //背景の色
    float clearColor[4] = { 0.0f, 0.5f, 0.5f, 1.0f };//R,G,B,A
    //画面をクリア
    pContext->ClearRenderTargetView(pRenderTargetView, clearColor);
	pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    
	//Imguiのフレーム開始
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Direct3D::EndDraw()
{
    ImGui::Button("Button");
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


    //スワップ（バックバッファを表に表示する）
    pSwapChain->Present(0, 0);
}

void Direct3D::Release()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();


    SAFE_RELEASE(pRasterizerState);
    SAFE_RELEASE(pVertexLayout);
    SAFE_RELEASE(pPixelShader);
    SAFE_RELEASE(pVertexShader);

    // ========== シャドウマップ解放 ==========
    SAFE_RELEASE(pShadowMapSRV);
    SAFE_RELEASE(pShadowMapDSV);
    SAFE_RELEASE(pShadowMapTexture);
    // ========================================
    SAFE_RELEASE(pDevice);		                    //デバイス
    SAFE_RELEASE(pContext);	                //デバイスコンテキスト
    SAFE_RELEASE(pSwapChain);		                //スワップチェイン
    SAFE_RELEASE(pRenderTargetView);	    //レンダーターゲットビュー
}

XMFLOAT4 Direct3D::GetLightPos()
{
    return lightPosition;
}

void Direct3D::SetLightPos(DirectX::XMFLOAT4 pos)
{
	lightPosition = pos;
}

// ライトのビュー行列を返す
// lightPosition は「光が来る方向ベクトル」なので、逆方向に仮想的な光源位置を置く
DirectX::XMMATRIX Direct3D::GetLightViewMatrix()
{
	XMVECTOR lightDir = XMLoadFloat4(&lightPosition);
	XMVECTOR lightEye = -XMVector3Normalize(lightDir) * 10.0f; // 原点から10離れた仮想ライト位置
	XMVECTOR lightAt  = XMVectorSet(0, 0, 0, 0);               // 注視点：シーンの中心
	XMVECTOR lightUp  = XMVectorSet(0, 1, 0, 0);               // 上方向

	return XMMatrixLookAtLH(lightEye, lightAt, lightUp);
}

// ライトの正射影行列を返す
// 平行光源なので XMMatrixOrthographicLH（透視投影ではない）
DirectX::XMMATRIX Direct3D::GetLightProjectionMatrix()
{
	float width  = 20.0f; // シーンをカバーする幅
	float height = 20.0f; // シーンをカバーする高さ
	float nearZ  =  1.0f; // 近クリップ面
	float farZ   = 50.0f; // 遠クリップ面

	return XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

// ========== シャドウマップ ==========

HRESULT Direct3D::InitShadowMap(int width, int height)
{
	HRESULT hr;

	// ① テクスチャ本体を作成（TYPELESS = 用途を後から決める）
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width          = width;
	texDesc.Height         = height;
	texDesc.MipLevels      = 1;
	texDesc.ArraySize      = 1;
	texDesc.Format         = DXGI_FORMAT_R32_TYPELESS;     // 型は後から決める
	texDesc.SampleDesc     = { 1, 0 };
	texDesc.Usage          = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL      // 深度として書く
						   | D3D11_BIND_SHADER_RESOURCE;   // シェーダーで読む
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags      = 0;

	hr = pDevice->CreateTexture2D(&texDesc, nullptr, &pShadowMapTexture);
	if (FAILED(hr)) {
		MessageBox(nullptr, L"ShadowMap Texture の作成に失敗しました", L"エラー", MB_OK);
		return hr;
	}

	// ② DSV（書き込み口）を作成
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format             = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	hr = pDevice->CreateDepthStencilView(pShadowMapTexture, &dsvDesc, &pShadowMapDSV);
	if (FAILED(hr)) {
		MessageBox(nullptr, L"ShadowMap DSV の作成に失敗しました", L"エラー", MB_OK);
		return hr;
	}

	// ③ SRV（読み込み口）を作成
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels       = 1;

	hr = pDevice->CreateShaderResourceView(pShadowMapTexture, &srvDesc, &pShadowMapSRV);
	if (FAILED(hr)) {
		MessageBox(nullptr, L"ShadowMap SRV の作成に失敗しました", L"エラー", MB_OK);
		return hr;
	}

	return S_OK;
}

ID3D11ShaderResourceView* Direct3D::GetShadowMapSRV()
{
	return pShadowMapSRV;
}

HRESULT Direct3D::InitShadowShader()
{
	HRESULT hr;

	// 頂点シェーダーをコンパイル
	ID3DBlob* pCompileVS = nullptr;
	D3DCompileFromFile(L"ShadowMap.hlsl", nullptr, nullptr,
		"VS", "vs_5_0", 0, 0, &pCompileVS, nullptr);
	assert(pCompileVS != nullptr);

	hr = pDevice->CreateVertexShader(
		pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(),
		nullptr, &shaderBundle[SHADER_SHADOWMAP].pVertexShader);
	if (FAILED(hr)) {
		MessageBox(nullptr, L"Shadow 頂点シェーダーの作成に失敗しました", L"エラー", MB_OK);
		return hr;
	}

	// ピクセルシェーダーをコンパイル
	ID3DBlob* pCompilePS = nullptr;
	D3DCompileFromFile(L"ShadowMap.hlsl", nullptr, nullptr,
		"PS", "ps_5_0", 0, 0, &pCompilePS, nullptr);
	assert(pCompilePS != nullptr);

	hr = pDevice->CreatePixelShader(
		pCompilePS->GetBufferPointer(), pCompilePS->GetBufferSize(),
		nullptr, &shaderBundle[SHADER_SHADOWMAP].pPixelShader);
	if (FAILED(hr)) {
		MessageBox(nullptr, L"Shadow ピクセルシェーダーの作成に失敗しました", L"エラー", MB_OK);
		return hr;
	}

	// 頂点レイアウト（POSITION だけ使う。UV や法線は不要）
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = pDevice->CreateInputLayout(layout, 1,
		pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(),
		&shaderBundle[SHADER_SHADOWMAP].pVertexLayout);
	if (FAILED(hr)) {
		MessageBox(nullptr, L"Shadow InputLayout の作成に失敗しました", L"エラー", MB_OK);
		return hr;
	}

	pCompileVS->Release();
	pCompilePS->Release();

	// ラスタライザー
	// CullMode を CULL_FRONT にするとシャドウアクネ（影のノイズ）が減る
	D3D11_RASTERIZER_DESC rdc = {};
	rdc.CullMode              = D3D11_CULL_FRONT;
	rdc.FillMode              = D3D11_FILL_SOLID;
	rdc.FrontCounterClockwise = FALSE;
	rdc.DepthClipEnable       = TRUE;
	pDevice->CreateRasterizerState(&rdc, &shaderBundle[SHADER_SHADOWMAP].pRasterizerState);

	return S_OK;
}

// ========== シャドウマップ END ==========

void Direct3D::BeginShadowPass()
{
    // シャドウマップの深度値を 1.0（最大値）でクリア
    pContext->ClearDepthStencilView(pShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // レンダーターゲットをシャドウマップ専用に切り替える
    // RTV = nullptr → 色は一切書かない
    // DSV = pShadowMapDSV → 深度だけシャドウマップに書く
    ID3D11RenderTargetView* nullRTV = nullptr;
    pContext->OMSetRenderTargets(1, &nullRTV, pShadowMapDSV);

    // ビューポートをシャドウマップのサイズに合わせる
    D3D11_TEXTURE2D_DESC texDesc;
    pShadowMapTexture->GetDesc(&texDesc);

    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)texDesc.Width;
    vp.Height   = (float)texDesc.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    pContext->RSSetViewports(1, &vp);

    // シャドウ専用シェーダーをセット
    SetShader(SHADER_SHADOWMAP);
}

void Direct3D::EndShadowPass()
{
    // レンダーターゲットを通常の画面（RTV + 深度ステンシル）に戻す
    pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);

    // ビューポートも画面サイズに戻す
    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)screenWidth;
    vp.Height   = (float)screenHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    pContext->RSSetViewports(1, &vp);
}
