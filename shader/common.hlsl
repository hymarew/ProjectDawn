#ifndef COMMON_HLSL
#define COMMON_HLSL

cbuffer WorldBuffer : register(b0)
{
	matrix World;
}
cbuffer ViewBuffer : register(b1)
{
	matrix View;
}
cbuffer ProjectionBuffer : register(b2)
{
	matrix Projection;
}

cbuffer CameraBuffer : register(b5)
{
	float4 CameraPosition;
}

struct MATERIAL
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;
	float4 Emission;
	float Shininess;
	bool TextureEnable;
	float2 Dummy;
};

cbuffer MaterialBuffer : register(b3)
{
	MATERIAL Material;
}

struct LIGHT
{
    bool Enable;
    bool CastShadow; // 追加: 影を落とすかどうかのフラグ
    bool2 Dummy;	//上のが増えたから3から2へ
    
	float4 Direction;
	float4 Diffuse;
	float4 Ambient;
	
    float4 Position;
    float4 PointLightParam; //ポイントライトの範囲
    float4 SpotLightParam; //スポットライトの角度
};

cbuffer LightBuffer : register(b4)
{
	LIGHT Light;
}

cbuffer LightViewProjectionBuffer : register(b6)
{
    matrix LightViewProjection;
}

struct VS_IN
{
	float4 Position		: POSITION0;
	float4 Normal		: NORMAL0;
	float4 Diffuse		: COLOR0;
	float2 TexCoord		: TEXCOORD0;

};

struct PS_IN
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION0;
    float4 Normal : NORMAL0;
    float4 Diffuse : COLOR0;
    float2 TexCoord : TEXCOORD0;
    float4 ShadowPos : TEXCOORD1;
};

#endif