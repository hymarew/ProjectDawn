#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float4 tex = g_Texture.Sample(g_SamplerState, In.TexCoord);

    // STEP5: ShadowMap Alpha Clip（葉の透明部分を影から除外）
    clip(tex.a - 0.5f);
}