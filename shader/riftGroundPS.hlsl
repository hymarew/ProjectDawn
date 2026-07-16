// riftGroundPS.hlsl : SpaceRiftの地面投影（CrackMaskをそのまま地面へ投影する）
// Emissionのみ使用し、Alphaは使わない（常にAdditiveブレンドで加算される想定）
#include "common.hlsl"
#include "riftParams.hlsl"

Texture2D    g_CrackMask    : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outColor : SV_Target0, out float4 outEmissive : SV_Target1)
{
    float2 uv = In.TexCoord;
    uv.x += Time * UVSpeed;

    float mask = RiftMaskIntensity(g_CrackMask.Sample(g_SamplerState, uv).rgb);

    if (mask < RIFT_ALPHA_CLIP_THRESHOLD)
        discard;

    float3 glow = OuterColor.rgb * mask * RiftPulse() * GlowStrength;

    outColor.rgb    = glow;
    outColor.a      = 1.0f; // Additiveブレンドのため実質未使用
    outEmissive.rgb = glow * BloomIntensity;
    outEmissive.a   = 1.0f;
}
