cbuffer cbShadow : register(b0)
{
    row_major float4x4 matLightWVP;  // ライト視点のWorld×View×Projection行列
};

// 頂点をライト視点に変換するだけ
float4 VS(float4 pos : POSITION) : SV_POSITION
{
    return mul(pos, matLightWVP);
}

// 何もしない
// GPU が SV_POSITION の Z 値を自動で深度バッファに書き込む
void PS(float4 pos : SV_POSITION)
{
}
