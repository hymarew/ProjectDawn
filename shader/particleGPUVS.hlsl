#include "common.hlsl"
#include "particleGPUCommon.hlsl"

// ===================================================
// particleGPUVS.hlsl
// GPUパーティクル（間接描画）用の頂点シェーダー
//
// 【CPU版 particleVS との違い】
//   - インスタンスデータを頂点ストリームからではなく、
//     Update CS が構築した StructuredBuffer（描画リスト）から
//     SV_InstanceID で直接読む（頂点バッファ・入力レイアウト不要）
//   - 板ポリの4頂点も SV_VertexID から生成する
//   - どのテクスチャを使うかは Flags 内のインデックスで
//     ピクセルシェーダーへ渡す（Texture2DArray 用）
// ===================================================

StructuredBuffer<ParticleDrawData> DrawList : register(t0);

struct PS_IN_PARTICLE_GPU
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
    nointerpolation uint TexIndex : TEXCOORD1; // Texture2DArray のスライス番号
};

void main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID,
          out PS_IN_PARTICLE_GPU Out)
{
    ParticleDrawData p = DrawList[instanceId];

    // ---- 板ポリの4頂点を SV_VertexID から生成（TRIANGLESTRIP） ----
    // CPU版の共有頂点バッファと同じ並び: 左上→右上→左下→右下
    float2 corner = float2( (vertexId & 1) ? 0.5f : -0.5f,
                            (vertexId & 2) ? -0.5f : 0.5f );
    float2 uv     = float2( (vertexId & 1) ? 1.0f : 0.0f,
                            (vertexId & 2) ? 1.0f : 0.0f );

    float c = cos(p.Rotation);
    float s = sin(p.Rotation);

    // サイズを適用したローカル座標
    float2 local = corner * p.Size;

    float3 offset;
    if (p.Flags & PARTICLE_FLAG_GROUND_ALIGNED)
    {
        // ---- 地面水平（爆風リング用）: particleVS と同じ変換 ----
        offset = float3(local.x * c + local.y * s,
                        0.0f,
                       -local.x * s + local.y * c);
    }
    else
    {
        // ---- ビルボード: 面内スピン後、ビュー回転の逆行列でカメラへ向ける ----
        float2 spun = float2(local.x * c - local.y * s,
                             local.x * s + local.y * c);

        float3x3 invViewRot = transpose((float3x3)View);
        offset = mul(float3(spun, 0.0f), invViewRot);
    }

    float4 worldPos = float4(p.Position + offset, 1.0f);

    Out.Position = mul(mul(worldPos, View), Projection);
    Out.Color    = p.Color;
    Out.TexCoord = uv;
    Out.TexIndex = (p.Flags >> PARTICLE_TEX_INDEX_SHIFT) & PARTICLE_TEX_INDEX_MASK;
}
