#include "common.hlsl"
#include "commonFanc.hlsl"

// メインテクスチャ
Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{

    if (Material.TextureEnable)
    {
        outDiffuse = g_Texture.Sample(g_SamplerState, In.TexCoord);
        outDiffuse *= In.Diffuse;
    }
    else
    {
        outDiffuse = In.Diffuse;
    }
    
    outDiffuse *= Material.Diffuse;
    
    float shadowFactor = CalcShadowFactor(In.ShadowPos);
    
    // 影の暗さを最終的な色に乗算
    outDiffuse.xyz *= shadowFactor;
}