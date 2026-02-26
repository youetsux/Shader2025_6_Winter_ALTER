//───────────────────────────────────────
// テクスチャ＆サンプラーデータのグローバル変数定義
//───────────────────────────────────────
Texture2D g_texture : register(t0); //テクスチャー
SamplerState g_sampler : register(s0); //サンプラー

Texture2D g_ToonTexture : register(t1); //テクスチャー
SamplerState g_ToonSampler : register(s1); //サンプラー



//───────────────────────────────────────
// コンスタントバッファ
// DirectX 側から送信されてくる、ポリゴン頂点以外の諸情報の定義
//───────────────────────────────────────
cbuffer global : register(b0)
{
    row_major float4x4 matWVP; // ワールド・ビュー・プロジェクションの合成行列
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
    float4 lightPosition;
    float4 eyePosition;
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
    
    // ライトベクトル計算（最適化版）
    float3 lightVec = lightPosition.xyz - inData.wpos.xyz;
    float len = length(lightVec);
    float3 L = lightVec / len; // 正規化（normalize(lightVec)と同じ）
    
    // ========== 距離減衰を有効化==========
    float3 k = { 0.2f, 0.2f, 1.0f };
    float dTerm = 1.0 / (k.x + k.y * len + k.z * len * len); // 距離減衰を有効化
    //float dTerm = 1.0;  // 距離減衰なし（デバッグ用）
    // ========== 距離減衰 END==========
    
    // 法線
    float3 N = normalize(inData.normal.xyz);
    
    // ディフューズ
    float ndotl = saturate(dot(N, L)); // NとLの内積を0～1にクランプ
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
    float4 texColor;
    if (useTexture == 1)
    {
        texColor = g_texture.Sample(g_sampler, inData.uv);
        diffuseTerm = diffuse * texColor;
        ambientTerm = ambentFactor * texColor;
    }
    else
    {
        diffuseTerm = diffuse;
        ambientTerm = diffuse * ambentFactor;
    }
    
    float2 toonUV = float2(ndotl, 0.0f); // ndotlをUVのx成分に使う
    float4 toonColor = g_ToonTexture.Sample(g_ToonSampler, toonUV); // トゥーンテクスチャから色をサンプリング
   
   
    diffuseTerm = diffuseTerm * toonColor;
    ambientTerm = ambentFactor;

    float4 color = diffuseTerm + specularCol + ambientTerm;
   
    //視線ベクトル　inData.eyev.xyz;
    //面の法線　inData.normal.xyz;
    float3 Vvec = normalize(-inData.eyev.xyz);
    float3 Nvec = normalize(inData.normal.xyz);
    float VdotN = abs(dot(Nvec, Vvec));
    if (VdotN < 0.15)
    {
        color = float4(0, 0, 0, 1);
    }
    return color * diffuseTerm + specularCol + ambientTerm;

}


    ////0, 1 / 4, 2 / 4, 3 / 4,  1
    
    //if (ndotl > (float) 3 / 4)
    //{
    //    color = float4(1, 1, 1, 1);
    //}
    //else if (ndotl > (float) 2 / 4)
    //{
    //    color = float4(0.8, 0.8, 0.8, 1);
    //}
    //else if (ndotl > (float) 1 / 4)
    //{
    //    color = float4(0.4, 0.4, 0.4, 1);
    //}
    //else
    //{
    //    color = float4(0.3, 0.3, 0.3, 1);
    //}