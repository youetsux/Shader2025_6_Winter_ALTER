////───────────────────────────────────────
//// テクスチャ＆サンプラーデータのグローバル変数定義
////───────────────────────────────────────
//Texture2D g_texture : register(t0); //テクスチャー
//SamplerState g_sampler : register(s0); //サンプラー

////───────────────────────────────────────
//// コンスタントバッファ
//// DirectX 側から送信されてくる、ポリゴン頂点以外の諸情報の定義
////───────────────────────────────────────
//cbuffer global : register(b0)
//{
//    row_major float4x4 matWVP; // ワールド・ビュー・プロジェクションの合成行列
//    row_major float4x4 matWorld; // ワールド行列
//    row_major float4x4 matNormal; // 法線変換行列
//    float4 diffuseColor; // ディフューズ色
//    float4 diffusefactor; // ディフューズ係数
//    float4 specular; // スペキュラ色
//    float4 shininess; // シャイニネス
//    float4 ambient; // アンビエント色
//    bool useTexture; // テクスチャーを使うかどうか
//};

//cbuffer gStage : register(b1)
//{
//    float4 lightPosition;
//    float4 eyePosition;
//};


////───────────────────────────────────────
//// 頂点シェーダー出力＆ピクセルシェーダー入力データ構造体
////───────────────────────────────────────
//struct VS_OUT
//{
//                 //セマンティクス
//    //float4 wpos : POSITION0; //ワールド座標
//    //float4 spos : SV_POSITION; //スクリーン位置
//    //float2 uv : TEXCOORD; //UV座標
//    //float4 normal : NORMAL; //法線ベクトル
//    //float4 eyev : POSITION1; //視線ベクトル
//    float4 spos : SV_POSITION; //スクリーン位置
//    float2 uv : TEXCOORD0; //UV座標
//    float4 wpos : TEXCOORD1; //ワールド座標
//    float4 normal : TEXCOORD2; //法線ベクトル
//    float4 eyev : TEXCOORD3; //視線ベクトル
//};

////───────────────────────────────────────
//// 頂点シェーダ
////───────────────────────────────────────
//VS_OUT VS(float4 pos : POSITION, float4 uv : TEXCOORD, float4 normal : NORMAL)
//{
//	//ピクセルシェーダーへ渡す情報
//    VS_OUT outData;

//	//ローカル座標に、ワールド・ビュー・プロジェクション行列をかけて
//    //スクリーン座標に変換し、ピクセルシェーダーへ
//    outData.spos = mul(pos, matWVP);
//    //ワールド座標もピクセルシェーダーへ
//    outData.wpos = mul(pos, matWorld);

//    normal.w = 0; //法線ベクトルのw成分は0にする
//    outData.normal = mul(normal, matNormal);
    
//    outData.uv = uv; //UV座標はそのまま
//    outData.eyev = outData.wpos - eyePosition; //視線ベクトルを計算して渡す

//	//まとめて出力
//    return outData;
//}

////───────────────────────────────────────
//// ピクセルシェーダ
////───────────────────────────────────────
//float4 PS(VS_OUT inData) : SV_Target
//{
//   // ========== デバッグ: 法線を可視化（コメントアウトを外すと法線確認可能） ==========
//    // float3 NC = normalize(inData.normal.xyz);
//    // return float4(NC * 0.5 + 0.5, 1); // 法線可視化（RGB = 法線XYZ）
//    // ========== デバッグ END ==========
    
//    float4 diffuse;
//    float4 ambientColor = ambient;
//    float4 ambentFactor = { 0.1, 0.1, 0.1, 1.0 };
    
//    // ライトベクトル計算（最適化版）
//    float3 lightVec = lightPosition.xyz - inData.wpos.xyz;
//    float len = length(lightVec);
//    float3 L = lightVec / len; // 正規化（normalize(lightVec)と同じ）
    
//    // ========== 距離減衰を有効化==========
//    float3 k = { 0.2f, 0.2f, 1.0f };
//    float dTerm = 1.0 / (k.x + k.y * len + k.z * len * len); // 距離減衰を有効化
//    //float dTerm = 1.0;  // 距離減衰なし（デバッグ用）
//    // ========== 距離減衰 END==========
    
//    // 法線
//    float3 N = normalize(inData.normal.xyz);
    
//    // ディフューズ
//    float ndotl = saturate(dot(N, L));
//    diffuse = diffuseColor * diffusefactor * ndotl * dTerm;
    
//    // スペキュラ
//    float spec = 0.0;
//    if (ndotl > 0.0)
//    {
//        float3 R = reflect(-L, N);
//        float3 V = normalize(-inData.eyev.xyz);
//        spec = pow(saturate(dot(R, V)), 32.0);
//    }
//    float4 specularCol = specular * spec * dTerm;
    
//    // テクスチャとの合成
//    float4 diffuseTerm;
//    float4 ambientTerm;
    
//    if (useTexture == 1)
//    {
//        float4 texColor = g_texture.Sample(g_sampler, inData.uv);
//        diffuseTerm = diffuse * texColor;
//        ambientTerm = ambentFactor * texColor;
//    }
//    else
//    {
//        diffuseTerm = diffuse;
//        ambientTerm = ambentFactor * diffuseColor;
//    }
    
//    float4 color = diffuseTerm + specularCol + ambientTerm;
//    return color;
//}

//───────────────────────────────────────
// テクスチャ＆サンプラーデータのグローバル変数定義
//───────────────────────────────────────
Texture2D g_texture : register(t0); // Diffuse テクスチャ
Texture2D g_normalMap : register(t1); // ノーマルマップ
SamplerState g_sampler : register(s0); // サンプラー

//───────────────────────────────────────
// コンスタントバッファ
//───────────────────────────────────────
cbuffer global : register(b0)
{
    row_major float4x4 matWVP;
    row_major float4x4 matWorld;
    row_major float4x4 matNormal;
    float4 diffuseColor;
    float4 diffusefactor;
    float4 specular;
    float4 shininess;
    float4 ambient;
    bool useTexture;
};

cbuffer gStage : register(b1)
{
    float4 lightPosition; // 点光源
    float4 eyePosition; // カメラ
    float4 spotLightPosition; // ← スポットライト位置
    float4 spotLightDirection; // ← スポットライト方向
    float4 spotLightParams; // ← x:内側角度cos, y:外側角度cos, z:減衰, w:未使用
};

//───────────────────────────────────────
// 頂点シェーダー出力＆ピクセルシェーダー入力データ構造体
//───────────────────────────────────────
struct VS_OUT
{
    float4 spos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 wpos : TEXCOORD1;
    float4 normal : TEXCOORD2;
    float4 tangent : TEXCOORD3; // ← 追加
    float4 binormal : TEXCOORD4; // ← 追加
    float4 eyev : TEXCOORD5;
};

//───────────────────────────────────────
// 頂点シェーダ
//───────────────────────────────────────
VS_OUT VS(float4 pos : POSITION,
          float2 uv : TEXCOORD,
          float4 normal : NORMAL,
          float4 tangent : TANGENT, // ← 追加
          float4 binormal : BINORMAL)  // ← 追加
{
    VS_OUT outData;

    outData.spos = mul(pos, matWVP);
    outData.wpos = mul(pos, matWorld);

    normal.w = 0;
    outData.normal = mul(normal, matNormal);
    
    // Tangent と Binormal の変換
    tangent.w = 0;
    outData.tangent = mul(tangent, matNormal);
    
    binormal.w = 0;
    outData.binormal = mul(binormal, matNormal);
    
    outData.uv = uv;
    outData.eyev = outData.wpos - eyePosition;

    return outData;
}


float4 PS(VS_OUT inData) : SV_Target
{
    // ========== ノーマルマップから法線を取得 ==========
    float3 normalMapSample = g_normalMap.Sample(g_sampler, inData.uv).rgb;
    float3 normalTangentSpace = normalMapSample * 2.0 - 1.0;
    
    float3 T = normalize(inData.tangent.xyz);
    float3 B = normalize(inData.binormal.xyz);
    float3 N = normalize(inData.normal.xyz);
    float3x3 TBN = float3x3(T, B, N);
    
    float3 worldNormal = normalize(mul(normalTangentSpace, TBN));
    // =================================================
    
    float4 ambentFactor = { 0.1, 0.1, 0.1, 1.0 };
    float3 k = { 0.2f, 0.2f, 1.0f };
    
    // ライティング結果を蓄積する変数
    float4 diffuse = float4(0, 0, 0, 0);
    float4 specularCol = float4(0, 0, 0, 0);
    
    // ========== 点光源のライティング ==========
    float3 lightVec = lightPosition.xyz - inData.wpos.xyz;
    float len = length(lightVec);
    float3 L = normalize(lightVec);
    
    float dTerm = 1.0 / (k.x + k.y * len + k.z * len * len);
    
    float ndotl = saturate(dot(worldNormal, L));
    diffuse += diffuseColor * diffusefactor * ndotl * dTerm;
    
    if (ndotl > 0.0)
    {
        float3 R = reflect(-L, worldNormal);
        float3 V = normalize(-inData.eyev.xyz);
        float spec = pow(saturate(dot(R, V)), 32.0);
        specularCol += specular * spec * dTerm;
    }
    // ===========================================
    
    
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
    return color;
}