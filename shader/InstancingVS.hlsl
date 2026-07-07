#include "Common.hlsl"

// ★インスタンシング専用の頂点入力構造体
struct VS_IN_INSTANCING
{
    float4 Position : POSITION0;
    float4 Normal : NORMAL0;
    float4 Diffuse : COLOR0;
    float2 TexCoord : TEXCOORD0;
	
    // インスタンスデータ（スロット1から来る4x4行列）
    float4 MatRow0 : TEXCOORD1;
    float4 MatRow1 : TEXCOORD2;
    float4 MatRow2 : TEXCOORD3;
    float4 MatRow3 : TEXCOORD4;
};

void main(in VS_IN_INSTANCING In, out PS_IN Out)
{
    // 行列の再構築
    matrix instanceWorld;
    instanceWorld[0] = In.MatRow0;
    instanceWorld[1] = In.MatRow1;
    instanceWorld[2] = In.MatRow2;
    instanceWorld[3] = In.MatRow3;

    // 行列計算
    matrix wvp;
    wvp = mul(instanceWorld, View);
    wvp = mul(wvp, Projection);
    
    // 頂点座標変換
    float4 worldPos = mul(In.Position, instanceWorld);
    Out.Position = mul(In.Position, wvp);
    Out.ShadowPos = mul(worldPos, LightViewProjection);

    // 法線変換
    float4 normal = float4(In.Normal.xyz, 0.0f);
    float4 worldNormal = mul(normal, instanceWorld);
    Out.Normal = normalize(worldNormal);

    // 出力
    Out.Diffuse = In.Diffuse;
    Out.TexCoord = In.TexCoord;
    Out.WorldPosition = worldPos;
}