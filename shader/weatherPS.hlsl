// ===================================================
// weatherPS.hlsl
// 天候パーティクル（雨・雪）のピクセルシェーダー
//
// AlphaBlend専用（Additiveは使わない）。
// FadeStart〜FadeEnd の距離でソフトにフェードアウトさせる（主に雪用。
// 雨はフェード無効の値を入れて実質無効化する）。
// ===================================================
#include "weatherParams.hlsl"

Texture2D    g_Texture      : register(t0);
SamplerState g_SamplerState : register(s0);

struct PS_IN_WEATHER
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
    float  ViewDist : TEXCOORD1;
};

void main(in PS_IN_WEATHER In, out float4 outDiffuse : SV_Target)
{
    outDiffuse = g_Texture.Sample(g_SamplerState, In.TexCoord) * In.Color;

    // 距離によるソフトなフェードアウト（遠くの粒がパッと消えるのを防ぐ）
    float fade = saturate((FadeEnd - In.ViewDist) / max(FadeEnd - FadeStart, 0.001f));
    outDiffuse.a *= fade;

    // ほぼ完全に透明なピクセルは破棄する
    if (outDiffuse.a < 0.01f)
    {
        discard;
    }
}
