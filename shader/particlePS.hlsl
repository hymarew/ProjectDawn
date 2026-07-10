// ===================================================
// particlePS.hlsl
// パーティクルのインスタンシング描画用ピクセルシェーダー
//
// 色はインスタンスデータ（頂点シェーダー経由）から受け取るため、
// 旧実装と違い Material 定数バッファを毎パーティクル更新する必要がない。
// ===================================================

Texture2D    g_Texture      : register(t0);
SamplerState g_SamplerState : register(s0);

struct PS_IN_PARTICLE
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

void main(in PS_IN_PARTICLE In, out float4 outDiffuse : SV_Target)
{
    outDiffuse = g_Texture.Sample(g_SamplerState, In.TexCoord) * In.Color;

    // ほぼ完全に透明なピクセルだけ破棄する（unlitTexturePS と同じしきい値）
    if (outDiffuse.a < 0.01f)
    {
        discard;
    }
}
