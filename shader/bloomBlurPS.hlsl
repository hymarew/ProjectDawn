// BloomBlur : Emissiveバッファをぼかす（水平パス→垂直パスの2回に分けて使う共通シェーダー）
// Direction  = ぼかす方向（水平パスは(1,0)、垂直パスは(0,1)）
// BlurRadius = ぼかし半径（ピクセル）
#include "common.hlsl"
#include "bloomParams.hlsl"

Texture2D    g_Texture      : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outColor : SV_Target)
{
    float2 uv    = In.TexCoord;
    float2 texel = Direction / ScreenSize;

    // blurPS.hlsl と同じ5タップのガウシアン風カーネル（左右対称に9タップ相当）
    const float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };

    float4 sum = g_Texture.Sample(g_SamplerState, uv) * weights[0];
    for (int i = 1; i < 5; i++)
    {
        float2 offset = texel * (float)i * BlurRadius;
        sum += g_Texture.Sample(g_SamplerState, uv + offset) * weights[i];
        sum += g_Texture.Sample(g_SamplerState, uv - offset) * weights[i];
    }

    outColor = sum;
}
