//───────────────────────────────────────
// テクスチャ＆サンプラーデータのグローバル変数定義
//───────────────────────────────────────
Texture2D g_texture : register(t0); //テクスチャー
SamplerState g_sampler : register(s0); //サンプラー

//───────────────────────────────────────
// コンスタントバッファ
// DirectX 側から送信されてくる、ポリゴン頂点以外の諸情報の定義
//───────────────────────────────────────
cbuffer global
{
    float4x4 matWorld; // ワールド行列
};

//───────────────────────────────────────
// 頂点シェーダー出力＆ピクセルシェーダー入力データ構造体
//───────────────────────────────────────
struct VS_OUT
{
                 //セマンティクス
    float4 pos : SV_POSITION; //位置
    float2 uv : TEXCOORD; //UV座標
};

//───────────────────────────────────────
// 頂点シェーダ
//───────────────────────────────────────
VS_OUT VS(float4 pos : POSITION, float4 uv : TEXCOORD)
{
	//ピクセルシェーダーへ渡す情報
    VS_OUT outData;

	//ローカル座標に、ワールド・ビュー・プロジェクション行列をかけて
	//スクリーン座標に変換し、ピクセルシェーダーへ
    outData.pos = mul(pos, matWorld);
    outData.uv = uv.xy; //UV座標はそのまま
   
	//まとめて出力
    return outData;
}

//───────────────────────────────────────
// ピクセルシェーダ
//───────────────────────────────────────
float4 PS(VS_OUT inData) : SV_Target
{
    float4 color = g_texture.Sample(g_sampler, inData.uv);
    //color.r = 1.0f - color.r; //赤チャンネルを反転
    //color.g = 1.0f - color.g; //緑チャンネルを反転
    //color.b = 1.0f - color.b; //青チャンネルを反転
    
    //①円の中心、半径を指定
    //②今書こうとしているピクセルと円の中心の距離を計算
    //③距離が半径より大きければ透明にする
    //画像の大きさ気にしなくていいかも！
    if ((int) inData.pos.x & 1) //奇数列の場合
    {
        color = float4(0.0f, 0.0f, 0.0f, color.a); //赤色に変更)
        
    }
    
    //float c = (color.r + color.g + color.b) / 3.0f;
    //float c = 0.3f*color.r + 0.59f*color.g + 0.11f*color.b;
    //color = float4(c, c, c, color.a); //グレースケール化
    
    
  
    return color;
}