//───────────────────────────────────────
// テクスチャ＆サンプラーデータのグローバル変数定義
//───────────────────────────────────────
Texture2D              g_texture      : register(t0); // モデルのテクスチャ
SamplerState           g_sampler      : register(s0); // 通常サンプラー

Texture2D              g_shadowMap    : register(t1); // シャドウマップ（深度テクスチャ）
SamplerComparisonState g_shadowSampler: register(s1); // 比較サンプラー

//───────────────────────────────────────
// コンスタントバッファ
// DirectX 側から送信されてくる、ポリゴン頂点以外の諸情報の定義
//───────────────────────────────────────
cbuffer global : register(b0)
{
    row_major float4x4  matWVP; // ワールド・ビュー・プロジェクションの合成行列
    row_major float4x4 matWorld; // ワールド行列
    row_major float4x4 matNormal; // 法線変換行列
    float4 diffuseColor; // ディフューズ色
    float4 diffusefactor; // ディフューズ係数
    float4 specular; // スペキュラ色
    float4 shininess; // シャイニネス
    float4 ambient; // アンビエント色
    bool useTexture; // テクスチャーを使うかどうか
};

cbuffer gStage : register(b1)
{
    float4             lightPosition;
    float4             eyePosition;
    int                lightType;   // 0=平行光源, 1=点光源
    float3             _pad;
    row_major float4x4 matLightVP;  // ライト視点の VP 行列
};


//───────────────────────────────────────
// 頂点シェーダー出力＆ピクセルシェーダー入力データ構造体
//───────────────────────────────────────
struct VS_OUT
{
                 //セマンティクス
    float4 wpos : POSITION0; //ワールド座標
    float4 spos : SV_POSITION; //スクリーン位置
    float2 uv : TEXCOORD; //UV座標
    float4 normal : NORMAL; //法線ベクトル
    float4 eyev : POSITION1; //視線ベクトル
};

//───────────────────────────────────────
// 頂点シェーダ
//───────────────────────────────────────
VS_OUT VS(float4 pos : POSITION, float4 uv : TEXCOORD, float4 normal : NORMAL)
{
	//ピクセルシェーダーへ渡す情報
    VS_OUT outData;

	//ローカル座標に、ワールド・ビュー・プロジェクション行列をかけて
    //スクリーン座標に変換し、ピクセルシェーダーへ
    outData.spos = mul(pos, matWVP);
    //ワールド座標もピクセルシェーダーへ
    outData.wpos = mul(pos, matWorld);

    
    normal.w = 0; //法線ベクトルのw成分は0にする
    outData.normal = mul(normal, matNormal);
    
    
    uv.w = 0; //w成分は0にする
    outData.uv = uv.xy; //UV座標はそのまま
    outData.eyev = outData.wpos - eyePosition; //視線ベクトルを計算して渡す

    //normal = mul(normal, matNormal); //法線ベクトルをワールド・ビュー・プロジェクション行列で変換
    //normal = normalize(normal); //法線ベクトルを正規化=長さ1に)
    //normal.w = 0; //w成分は0にする
    //float4 light = float4(-1, 0.5, -0.7, 0);
    //light = normalize(light);
    //light.w = 0;
    //outData.color = clamp(dot(normal, light), 0, 1);

	//まとめて出力
    return outData;
}

//───────────────────────────────────────
// ピクセルシェーダ
//───────────────────────────────────────
float4 PS(VS_OUT inData) : SV_Target
{
    // ========== デバッグ: 法線を可視化（コメントアウトを外すと法線確認可能） ==========
    // float3 NC = normalize(inData.normal.xyz);
    // return float4(NC * 0.5 + 0.5, 1); // 法線可視化（RGB = 法線XYZ）
    // ========== デバッグ END ==========
    
    float4 diffuse;
    float4 ambientColor = ambient;
    float4 ambentFactor = { 0.1, 0.1, 0.1, 1.0 };

    float3 L;
    float dTerm;

    if (lightType == 0)
    {
        // ========== 平行光源 ==========
        // lightPosition を方向ベクトルとして使用（位置ではなく向き）
        L = normalize(lightPosition.xyz);
        dTerm = 1.0;  // 距離減衰なし
    }
    else
    {
        // ========== 点光源 ==========
        float3 lightVec = lightPosition.xyz - inData.wpos.xyz;
        float len = length(lightVec);
        L = lightVec / len;
        float3 k = { 0.2f, 0.2f, 1.0f };
        dTerm = 1.0 / (k.x + k.y * len + k.z * len * len);
    }
    
    // 法線
    float3 N = normalize(inData.normal.xyz);
    
    // ディフューズ
    float ndotl = saturate(dot(N, L));
    diffuse = diffuseColor * diffusefactor * ndotl * dTerm;
    
    // スペキュラ
    float spec = 0.0;
    if (ndotl > 0.0)
    {
        float3 R = reflect(-L, N);
        float3 V = normalize(-inData.eyev.xyz);
        spec = pow(saturate(dot(R, V)), 32.0);
    }
    float4 specularCol = specular * spec * dTerm;
    
    // テクスチャとの合成
    float4 diffuseTerm;
    float4 ambientTerm;
    
    if (useTexture == 1)
    {
        float4 texColor = g_texture.Sample(g_sampler, inData.uv);
        diffuseTerm = diffuse * texColor;
        ambientTerm = ambentFactor * texColor;
    }
    else
    {
        diffuseTerm = diffuse;
        ambientTerm = ambentFactor * diffuseColor;
    }
    
    float4 color = diffuseTerm + specularCol + ambientTerm;

    // ========== シャドウ判定 ==========
    float shadow = 1.0; // デフォルト: 影なし（明るい）

    // ① ワールド座標をライト視点のクリップ空間に変換
    float4 lightClipPos = mul(inData.wpos, matLightVP);

    // ② クリップ座標 → UV 座標に変換
    // クリップ空間は -1?+1、UV 空間は 0?1（Y軸は反転）
    float2 shadowUV;
    shadowUV.x =  lightClipPos.x / lightClipPos.w * 0.5 + 0.5;
    shadowUV.y = -lightClipPos.y / lightClipPos.w * 0.5 + 0.5;

    // ③ UV が 0?1 の範囲内にある場合のみ判定（範囲外 = ライトの視野外）
    if (shadowUV.x >= 0.0 && shadowUV.x <= 1.0 &&
        shadowUV.y >= 0.0 && shadowUV.y <= 1.0)
    {
        // ④ 現在のピクセルの深度値
        float currentDepth = lightClipPos.z / lightClipPos.w;

        // ⑤ シャドウバイアス
        // GREATER_EQUAL では compareValue = currentDepth - bias にすることで
        // ⑤ シャドウバイアス（自己シャドウ＝ドーナツが自分に影を落とすアクネを防ぐ）
        // LESS_EQUAL + bias: compare = currentDepth - bias → bias 分だけ"手前"と見なして判定
        // 大きすぎると床の影が消える（床との深度差 ? 0.007 を超えないこと）
        float bias = 0.005;

        // ⑥ 比較サンプラーで深度を比較
        // LESS_EQUAL: (currentDepth - bias) <= shadowMap なら 1.0（明るい）
        shadow = g_shadowMap.SampleCmpLevelZero(
            g_shadowSampler, shadowUV, currentDepth - bias);
    }

    // ⑦ 影の適用（影の中は 30% の明るさを残す）
    color *= (0.3 + 0.7 * shadow);
    // ========== シャドウ判定 END ==========

    return color;
}