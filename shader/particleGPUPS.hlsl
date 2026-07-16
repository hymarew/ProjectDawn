// ===================================================
// particleGPUPS.hlsl
// GPUパーティクル（間接描画）用のピクセルシェーダー
//
// 全パーティクル用テクスチャを1つの Texture2DArray に統合してあるため、
// テクスチャの切り替え（=ドローコールの分割）が不要になる。
// どのスライスを読むかは頂点シェーダーから渡されるインデックスで決まる。
// ===================================================

Texture2DArray g_TextureArray : register(t0);
SamplerState   g_SamplerState : register(s0);

struct PS_IN_PARTICLE_GPU
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
    nointerpolation uint TexIndex : TEXCOORD1;
};

void main(in PS_IN_PARTICLE_GPU In, out float4 outDiffuse : SV_Target)
{
    outDiffuse = g_TextureArray.Sample(g_SamplerState,
                     float3(In.TexCoord, (float)In.TexIndex)) * In.Color;

    // ほぼ完全に透明なピクセルだけ破棄する（particlePS と同じしきい値）
    if (outDiffuse.a < 0.01f)
    {
        discard;
    }
}
