//───────────────────────────────────────
// Sky Shader (Cubemap)
//───────────────────────────────────────
TextureCube g_skyTexture : register(t0);
SamplerState g_sampler : register(s0);

//───────────────────────────────────────
// コンスタントバッファ
//───────────────────────────────────────
cbuffer SkyConstants : register(b0)
{
    row_major float4x4 invProj;      // 逆プロジェクション行列
    row_major float4x4 invViewRot;   // 逆ビュー行列（回転のみ）
};

//───────────────────────────────────────
// 頂点シェーダー出力＆ピクセルシェーダー入力
//───────────────────────────────────────
struct VS_OUT
{
    float4 pos : SV_POSITION;
    float3 viewDir : TEXCOORD0;
};

//───────────────────────────────────────
// 頂点シェーダー（頂点バッファ不要）
//───────────────────────────────────────
VS_OUT VS(uint id : SV_VertexID)
{
    VS_OUT output;
    
    // フルスクリーントライアングル生成（ビット演算を使わない方法）
    float2 uv = float2(0.0, 0.0);
    if (id == 0) uv = float2(0.0, 0.0);
    else if (id == 1) uv = float2(2.0, 0.0);
    else if (id == 2) uv = float2(0.0, 2.0);
    
    output.pos = float4(uv * float2(2, -2) + float2(-1, 1), 1, 1);
    
    // NDC座標からビュー空間の方向ベクトルを計算
    float4 viewPos = mul(float4(output.pos.xy, 1, 1), invProj);
    viewPos.xyz /= viewPos.w;
    
    // ビュー空間からワールド空間へ（回転のみ）
    output.viewDir = mul(float4(viewPos.xyz, 0), invViewRot).xyz;
    
    return output;
}

//───────────────────────────────────────
// ピクセルシェーダー
//───────────────────────────────────────
float4 PS(VS_OUT input) : SV_Target
{
    return g_skyTexture.Sample(g_sampler, normalize(input.viewDir));
}
