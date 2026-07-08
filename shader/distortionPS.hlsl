// Distortion : 実際の画面を手続き型ノイズでUV歪みさせながら覆う演出（熱波/ワープ風）
// Progress = 0〜1（中間で歪みが最大になり、覆いきる頃には Color で覆われる）
// Param1   = 歪みの強さ
// Param2   = ノイズのスケール
// Param3   = 時間（アニメーションのオフセットに使う）
#include "common.hlsl"
#include "transitionCommon.hlsl"

Texture2D    g_ScreenTexture : register(t0);
SamplerState g_SamplerState  : register(s0);

// ---- 手続き型ノイズ（テクスチャ資産を使わないための簡易Value Noise） ----
float Hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453123f);
}

float Noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = Hash(i);
    float b = Hash(i + float2(1.0f, 0.0f));
    float c = Hash(i + float2(0.0f, 1.0f));
    float d = Hash(i + float2(1.0f, 1.0f));

    float2 u = f * f * (3.0f - 2.0f * f);
    return lerp(a, b, u.x) + (c - a) * u.y * (1.0f - u.x) + (d - b) * u.x * u.y;
}

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float2 uv = In.TexCoord;

    // 中間(Progress=0.5)で歪みが最大になり、両端では歪まない
    float strength = Param1 * saturate(sin(Progress * 3.14159265f));
    float scale    = max(Param2, 0.0001f);

    float2 noiseUV = uv * scale + Param3;
    float n1 = Noise(noiseUV) - 0.5f;
    float n2 = Noise(noiseUV + float2(5.2f, 1.3f)) - 0.5f;

    float2 distortedUV = saturate(uv + float2(n1, n2) * strength);

    outDiffuse     = g_ScreenTexture.Sample(g_SamplerState, distortedUV);
    outDiffuse.rgb = lerp(outDiffuse.rgb, Color.rgb, saturate(Progress));
    outDiffuse.a   = 1.0f;
}
