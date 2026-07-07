#include "Common.hlsl"

Texture2D g_Texture : register(t0);

SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    //=====================================================
    // ライト方向ベクトル
    //=====================================================
    // ピクセル → ライト
    //=====================================================

    float3 ligDir =
        Light.Position.xyz - In.WorldPosition.xyz;

    //=====================================================
    // ライトまでの距離
    //=====================================================

    float distance = length(ligDir);

    //=====================================================
    // 正規化
    //=====================================================

    ligDir = normalize(ligDir);

    //=====================================================
    // スポットライト方向
    //=====================================================
    // ライトが向いている方向
    //=====================================================

    float3 spotDir =
        normalize(Light.Direction.xyz);

    //=====================================================
    // スポットライト角度判定
    //=====================================================
    // dot値が大きいほど
    // ライト方向と近い
    //=====================================================

    float spot =
        dot(-ligDir, spotDir);

    //=====================================================
    // スポットライト減衰
    //=====================================================

    // inner cone
    float inner =
    Light.SpotLightParam.x;

    // outer cone
    float outer =
    Light.SpotLightParam.y;

    // 0～1へ変換
    float spotFactor =
    (spot - outer) / (inner - outer);

    // 範囲制限
    spotFactor = saturate(spotFactor);
    
    

    //=====================================================
    // diffuse
    //=====================================================

    float diff =
        dot(In.Normal.xyz, ligDir);

    diff = saturate(diff);

    //=====================================================
    // 距離減衰
    //=====================================================

    float range =
        Light.PointLightParam.x;

    float atten =
        1.0f - (distance / range);

    atten = saturate(atten);

    atten = pow(atten, 2.0f);

    //=====================================================
    // diffuse色
    //=====================================================

    float3 diffuse =
        Light.Diffuse.rgb *
        diff *
        atten *
        spotFactor;

    //=====================================================
    // カメラ方向
    //=====================================================

    float3 toEye =
        normalize(
            CameraPosition.xyz -
            In.WorldPosition.xyz
        );

    //=====================================================
    // 反射ベクトル
    //=====================================================

    float3 reflectVec =
        reflect(-ligDir, In.Normal.xyz);

    //=====================================================
    // specular
    //=====================================================

    float spec =
        dot(reflectVec, toEye);

    spec = saturate(spec);

    spec = pow(spec, 16.0f);

    //=====================================================
    // specular色
    //=====================================================

    float3 specular =
        Light.Diffuse.rgb *
        spec *
        atten *
        spotFactor;

    //=====================================================
    // テクスチャ取得
    //=====================================================

    float4 texColor = float4(1, 1, 1, 1);

    if (Material.TextureEnable)
    {
        texColor =
        g_Texture.Sample(
            g_SamplerState,
            In.TexCoord
        );
    }

    outDiffuse = texColor;

    //=====================================================
    // 最終カラー
    //=====================================================

    outDiffuse.rgb *= Material.Diffuse.rgb;

    outDiffuse.rgb *=
(
    diffuse +
    specular +
    Light.Ambient.rgb
);
}