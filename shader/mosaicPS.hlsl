// Mosaic : 実際の画面をブロック状に粗くしながら覆う演出
// Progress = 0〜1（進行するほどブロックが粗くなり、最終的に Color で覆われる）
// Param1   = 最大ブロックサイズ（ピクセル）
#include "common.hlsl"
#include "transitionCommon.hlsl"

Texture2D    g_ScreenTexture : register(t0);
SamplerState g_SamplerState  : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float2 uv = In.TexCoord;

    float blockSize = max(lerp(1.0f, Param1, Progress), 1.0f);

    // ブロック単位に丸めたUVで画面をサンプリングする（＝モザイク化）
    float2 blockUV = floor(uv * ScreenSize / blockSize) * blockSize / ScreenSize;
    blockUV += (blockSize * 0.5f) / ScreenSize; // ブロック中心をサンプル

    outDiffuse     = g_ScreenTexture.Sample(g_SamplerState, blockUV);
    outDiffuse.rgb = lerp(outDiffuse.rgb, Color.rgb, saturate(Progress));
    outDiffuse.a   = 1.0f; // 画面そのものを描き直しているので常に不透明
}
