#include "Common.hlsl"   // 必ずインクルード

void main(in VS_IN In, out PS_IN Out)
{
    // 頂点変換（必ず必要）
    matrix wvp; // ワールドビュープロジェクション行列
    wvp = mul(World, View); // wvp = ワールド行列＊カメラ行列
    wvp = mul(wvp, Projection); // wvp = ワwvp＊プロジェクション行列
    float4 worldPos = mul(In.Position, World); // ワールド座標
    Out.Position = mul(In.Position, wvp); // 頂点座標を行列で変換して出力

    //========================
    // 法線の回転処理
    //========================
    
    //頂点法線をワールド行列で回転させる(頂点と同じ回転をさせる)
    float4 worldNormal, normal; //ローカル変数を作成
    normal = float4(In.Normal.xyz, 0.0f); // In.Normal をコピー（w=0 にすることで平行移動の影響を受けない）
    worldNormal = mul(normal, World); // //コピーされた法線をワールド行列で回転する 
    worldNormal = normalize(worldNormal); //回転後の法線を正規化する
    Out.Normal = worldNormal; // 回転後の法線を出力　In.Normalでなく回転後の法線を出力

    //========================
    // 出力
    //========================
    
    Out.Diffuse = In.Diffuse; //頂点カラーをそのまま出力
    Out.TexCoord = In.TexCoord; //テクスチャの座標を出力
    Out.WorldPosition = worldPos;
}