// Blur : 実際の画面をガウシアン風にぼかしながら覆う演出（水平/垂直2パスで使う共通シェーダー）
// Progress   = 0〜1（進行するほどぼけが強くなり、最終的に Color で覆われる）
// Param1,2   = ぼかす方向（水平パスは(1,0)、垂直パスは(0,1)）
// Param3     = ぼかし半径（ピクセル）
#include "common.hlsl"
#include "transitionCommon.hlsl"

Texture2D    g_ScreenTexture : register(t0);
SamplerState g_SamplerState  : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float2 uv    = In.TexCoord;
    float2 dir   = float2(Param1, Param2);
    float2 texel = dir / ScreenSize;

    // 5タップのガウシアン風カーネル（左右対称に9タップ相当）
    const float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };

    float4 sum = g_ScreenTexture.Sample(g_SamplerState, uv) * weights[0];
    for (int i = 1; i < 5; i++)
    {
        float2 offset = texel * (float)i * Param3;
        sum += g_ScreenTexture.Sample(g_SamplerState, uv + offset) * weights[i];
        sum += g_ScreenTexture.Sample(g_SamplerState, uv - offset) * weights[i];
    }

    outDiffuse     = sum;
    outDiffuse.rgb = lerp(outDiffuse.rgb, Color.rgb, saturate(Progress));
    outDiffuse.a   = 1.0f;
}
