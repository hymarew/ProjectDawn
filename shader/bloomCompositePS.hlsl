// BloomComposite : ぼかし済みEmissiveをIntensity倍してそのまま出力する。
// 呼び出し側（BloomPass::Composite）でAdditiveブレンドを有効にしてバックバッファへ加算すること。
#include "common.hlsl"
#include "bloomParams.hlsl"

Texture2D    g_Texture      : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outColor : SV_Target)
{
    float4 c = g_Texture.Sample(g_SamplerState, In.TexCoord);
    outColor = float4(c.rgb * Intensity, 1.0f);
}
