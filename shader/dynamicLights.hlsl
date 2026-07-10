#ifndef DYNAMIC_LIGHTS_HLSL
#define DYNAMIC_LIGHTS_HLSL

// =====================================================
// 動的ポイントライト（ロケットの噴射炎・爆発フラッシュ等に使う軽量な追加光源）
//
// 従来の LightBuffer(b4) は「太陽光1灯 + 影」専用で、複数の光源を同時に
// 扱えないため、これとは別に小さな固定数のポイントライト配列を用意する。
// C++側は dynamicLightManager.h の DynamicLightsCB とレイアウトを一致させること。
// =====================================================

#define MAX_DYNAMIC_LIGHTS 4

struct DynamicLight
{
    float4 PositionRadius; // xyz = ワールド座標, w = 半径（これを超えると寄与ゼロ）
    float4 ColorIntensity; // xyz = 色,           w = 強さ
};

cbuffer DynamicLightsBuffer : register(b10)
{
    DynamicLight DynamicLights[MAX_DYNAMIC_LIGHTS];
    int    DynamicLightCount;
    float3 _DynamicLightsPad;
}

// 全ポイントライトの寄与を合算する（半径に応じた二次減衰、影は考慮しない簡易版）
float3 CalcDynamicLights(float3 worldPos, float3 normal)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < DynamicLightCount; i++)
    {
        float3 lightPos = DynamicLights[i].PositionRadius.xyz;
        float  radius   = DynamicLights[i].PositionRadius.w;
        if (radius <= 0.0001f) continue;

        float3 toLight = lightPos - worldPos;
        float  dist    = length(toLight);
        if (dist >= radius) continue;

        float3 L     = toLight / max(dist, 0.0001f);
        float  NdotL = saturate(dot(normal, L));

        float atten = saturate(1.0f - (dist / radius));
        atten *= atten; // 二次減衰で自然に消える

        result += DynamicLights[i].ColorIntensity.xyz * DynamicLights[i].ColorIntensity.w * NdotL * atten;
    }

    return result;
}

#endif
