
#pragma once
#include "Direct3D.h"
#include <DirectXMath.h>
#include <string>

using namespace DirectX;

struct CONSTANT_BUFFER
{
    DirectX::XMMATRIX matWorld; // ワールド行列
};

struct VERTEX
{
    XMFLOAT4 pos; // 位置
    XMFLOAT2 uv;  // UV座標
};

// 前方宣言
class Texture;

class Sprite
{
public:
    // 画像ファイル名（ANSI/UTF-8）を受け取るコンストラクタ
    explicit Sprite(const char* filename);

    // 画像ファイル名（UTF-16）を受け取るコンストラクタ（必要なら使用）
    explicit Sprite(const std::wstring& filename);

    virtual ~Sprite();

    // 描画
    virtual void Draw(XMMATRIX& worldMatrix);

    // リソース解放
    void Release();

    // コピー不可（必要ならムーブ可）
    Sprite(const Sprite&) = delete;
    Sprite& operator=(const Sprite&) = delete;

    // ムーブは任意（ここでは禁止）。必要なら実装してください。
    // Sprite(Sprite&&) = delete;
    // Sprite& operator=(Sprite&&) = delete;

protected:
    ID3D11Buffer* pVertexBuffer_;   // 頂点バッファ
    ID3D11Buffer* pIndexBuffer_;    // インデックスバッファ
    ID3D11Buffer* pConstantBuffer_; // 定数バッファ
    Texture* pTexture_;        // テクスチャ

    // 共通のバッファ初期化ロジック
    HRESULT InitializeBuffers();

    // テクスチャロード（文字コード別）
    void LoadTexture(const char* filename);
    void LoadTexture(const std::wstring& filename);
};
