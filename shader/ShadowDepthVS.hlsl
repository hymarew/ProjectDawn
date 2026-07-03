#include "common.hlsl"

void main(in VS_IN In, out PS_IN Out)
{
    // モデルの頂点をワールド空間へ変換
    float4 worldPos = mul(In.Position, World);
    
    // ワールド空間の座標を、ライトのView-Projection行列で変換
    Out.Position = mul(worldPos, LightViewProjection);
    Out.TexCoord = In.TexCoord;
    Out.Diffuse = In.Diffuse * Material.Diffuse;
}