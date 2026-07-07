#include "common.hlsl"
#include "commonFanc.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    // --- ベースカラー ---
    float4 baseColor;
    if (Material.TextureEnable)
    {
        baseColor = g_Texture.Sample(g_SamplerState, In.TexCoord);
        baseColor *= In.Diffuse;
    }
    else
    {
        baseColor = In.Diffuse;
    }
    baseColor *= Material.Diffuse;

    // アルファが低すぎるピクセルは描画しない
    if (baseColor.a < 0.1f)
        discard;

    // --- フォン反射モデルによるスペキュラー計算 ---

    float3 N = normalize(In.Normal.xyz);
    float3 L = normalize(-Light.Direction.xyz);
    float3 V = normalize(CameraPosition.xyz - In.WorldPosition.xyz);

    float3 specColor = CalcCrystalSpecular(N, L, V, Material.Specular.xyz);

    // --- 影の計算 ---
    float shadowFactor = CalcShadowFactor(In.ShadowPos);

    // クリスタルは影の中でも自己発光気味に光る
    float specFactor = max(shadowFactor, 0.4f);
    outDiffuse.xyz = baseColor.xyz * shadowFactor + specColor * specFactor;
    outDiffuse.a   = baseColor.a;
}
