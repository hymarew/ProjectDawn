#include "Common.hlsl"   // World/View/Projection/LightViewProjection/PS_IN を流用

// ボーン変形(スキニング)専用の頂点シェーダー。
// 既存のShadowMapLightingVS/common.hlslのVS_INは変更せず、
// スキニングモデル専用の入力(BoneIndices/BoneWeights)とボーン行列バッファのみをここで追加する。

#define MAX_BONES 128

cbuffer BoneMatrixBuffer : register(b8)
{
	matrix BoneMatrices[MAX_BONES];
}

struct VS_IN_SKIN
{
	float4 Position    : POSITION0;
	float4 Normal      : NORMAL0;
	float4 Diffuse     : COLOR0;
	float2 TexCoord    : TEXCOORD0;
	uint4  BoneIndices : BLENDINDICES0;
	float4 BoneWeights : BLENDWEIGHT0;
};

void main(in VS_IN_SKIN In, out PS_IN Out)
{
	//==================================================
	// ボーン行列をウェイトでブレンドしてスキニング行列を作る
	//==================================================
	matrix skinMatrix =
		BoneMatrices[In.BoneIndices.x] * In.BoneWeights.x +
		BoneMatrices[In.BoneIndices.y] * In.BoneWeights.y +
		BoneMatrices[In.BoneIndices.z] * In.BoneWeights.z +
		BoneMatrices[In.BoneIndices.w] * In.BoneWeights.w;

	float4 skinnedPosition = mul(In.Position, skinMatrix);
	float4 skinnedNormal   = mul(float4(In.Normal.xyz, 0.0f), skinMatrix);

	//==================================================
	// 以降はShadowMapLightingVSと同じ流れ(World/View/Projection適用)
	//==================================================
	matrix wvp = mul(World, View);
	wvp = mul(wvp, Projection);

	float4 worldPos = mul(skinnedPosition, World);
	Out.Position  = mul(skinnedPosition, wvp);
	Out.ShadowPos = mul(worldPos, LightViewProjection);

	float4 worldNormal = mul(skinnedNormal, World);
	worldNormal = normalize(worldNormal);
	Out.Normal = worldNormal;

	Out.Diffuse = In.Diffuse;
	Out.TexCoord = In.TexCoord;
	Out.WorldPosition = worldPos;
}
