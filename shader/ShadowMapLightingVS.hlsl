#include "Common.hlsl"   // 必ずインクルード

void main(in VS_IN In, out PS_IN Out)
{
    
    //==================================================
    // 行列計算
    //==================================================
    matrix wvp;
    wvp = mul(World, View);
    wvp = mul(wvp, Projection);
    
    //==================================================
    // 頂点座標変換
    //==================================================
    float4 worldPos = mul(In.Position, World);
    Out.Position = mul(In.Position, wvp);
    Out.ShadowPos = mul(worldPos, LightViewProjection); //シャドウマップ座標変換

    //==================================================
    // 法線変換
    //==================================================
    float4 worldNormal, normal;
    normal = float4(In.Normal.xyz, 0.0f); // w = 0 にすることで平行移動の影響を受けない
    worldNormal = mul(normal, World); // ワールド行列で回転
    worldNormal = normalize(worldNormal);
    Out.Normal = worldNormal;

    //==================================================
    // 出力
    //==================================================

    Out.Diffuse = In.Diffuse;
    Out.TexCoord = In.TexCoord;
    Out.WorldPosition = worldPos;
}