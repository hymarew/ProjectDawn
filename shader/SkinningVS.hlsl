#include "Common.hlsl"

// Phase2: GPUスキニング用頂点シェーダー。
// 既存のVS_IN/CreateVertexShader(VERTEX_3D専用)には手を入れず、
// ボーン付きモデル専用の入力レイアウト+cbufferを追加する形で対応する。

#define MAX_BONES 128 // fbxModelRenderer.h の FbxSkinData::MAX_BONES と一致させること

cbuffer BoneMatrixBuffer : register(b8)
{
    matrix BoneMatrices[MAX_BONES];
}

struct VS_IN_SKIN
{
    float4 Position     : POSITION0;
    float4 Normal       : NORMAL0;
    float4 Diffuse      : COLOR0;
    float2 TexCoord     : TEXCOORD0;
    uint4  BoneIndices  : BLENDINDICES0;
    float4 BoneWeights  : BLENDWEIGHT0;
};

void main(in VS_IN_SKIN In, out PS_IN Out)
{
    //==================================================
    // スキニング: 最大4ボーンの加重平均でローカル座標・法線を変形
    //==================================================
    float4 skinnedPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 skinnedNormal   = float3(0.0f, 0.0f, 0.0f);

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float weight = In.BoneWeights[i];
        matrix boneMatrix = BoneMatrices[In.BoneIndices[i]];

        skinnedPosition += weight * mul(In.Position, boneMatrix);
        skinnedNormal   += weight * mul(In.Normal.xyz, (float3x3)boneMatrix);
    }
    skinnedPosition.w = 1.0f;

    //==================================================
    // 行列計算
    //==================================================
    matrix wvp;
    wvp = mul(World, View);
    wvp = mul(wvp, Projection);

    //==================================================
    // 頂点座標変換
    //==================================================
    float4 worldPos = mul(skinnedPosition, World);
    Out.Position = mul(skinnedPosition, wvp);
    Out.ShadowPos = mul(worldPos, LightViewProjection);

    //==================================================
    // 法線変換
    //==================================================
    float4 worldNormal = mul(float4(skinnedNormal, 0.0f), World);
    worldNormal = normalize(worldNormal);
    Out.Normal = worldNormal;

    //==================================================
    // 出力
    //==================================================
    Out.Diffuse = In.Diffuse;
    Out.TexCoord = In.TexCoord;
    Out.WorldPosition = worldPos;
}
