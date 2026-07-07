#include "common.hlsl"
#include "commonFanc.hlsl"

Texture2D    g_Texture      : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    outDiffuse  = g_Texture.Sample(g_SamplerState, In.TexCoord);
    outDiffuse *= In.Diffuse;
    outDiffuse *= Material.Diffuse;

    // Alpha Clip（葉テクスチャの透明部分を破棄）
    if (outDiffuse.a < 0.5f)
        discard;

    float shadowFactor = CalcShadowFactor(In.ShadowPos);
    outDiffuse.xyz *= shadowFactor;
}