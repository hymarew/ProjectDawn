#include "common.hlsl"
#include "commonFanc.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

// ヒットフラッシュ用（被弾した瞬間モデル全体を白く発光させる）
// C++側は Scorpion::Draw() が個体ごとの FlashIntensity を書き込む
cbuffer FlashBuffer : register(b9)
{
    float  FlashIntensity; // 0=通常表示, 1=真っ白
    float3 FlashDummy;
}

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    // ベースカラー（テクスチャまたは頂点カラー × マテリアルDiffuse）
    float4 baseColor;
    if (Material.TextureEnable)
        baseColor = g_Texture.Sample(g_SamplerState, In.TexCoord) * In.Diffuse;
    else
        baseColor = In.Diffuse;

    baseColor *= Material.Diffuse;

    if (baseColor.a < 0.1f)
        discard;

    // ライティングベクトル
    float3 N = normalize(In.Normal.xyz);
    float3 L = normalize(-Light.Direction.xyz);
    float3 V = normalize(CameraPosition.xyz - In.WorldPosition.xyz);
    float3 H = normalize(L + V);

    // --- ディフューズ（Lambert） ---
    float NdotL = max(dot(N, L), 0.0f);

    // MTLのKdが極端に暗い場合の最低輝度保証
    float3 baseMin = float3(0.12f, 0.08f, 0.05f);
    float3 visibleBase = max(baseColor.xyz, baseMin);

    // 半球ライティング（Blenderのビューポートに近い環境光）
    // 法線のY成分で上向き面と下向き面を判別し、異なる空・地面色を与える
    float3 skyColor    = float3(0.28f, 0.32f, 0.40f); // 青みがかった空
    float3 groundColor = float3(0.04f, 0.03f, 0.02f); // 暗い地面
    float  hemi        = dot(N.xyz, float3(0.0f, 1.0f, 0.0f)) * 0.5f + 0.5f;
    float3 hemisphereAmbient = lerp(groundColor, skyColor, hemi);

    // ディフューズ = 方向ライト + 半球環境光
    float3 diffuse = visibleBase * (Light.Diffuse.xyz * NdotL + hemisphereAmbient);

    // --- 鏡面金属スペキュラー ---
    float NdotH = max(dot(N, H), 0.0f);

    // 鋭ハイライト：外骨格の控えめな光点
    float sharpSpec = pow(NdotH, 64.0f) * 1.5f;

    // 広ハイライト：面全体のうっすらした光沢
    float broadSpec = pow(NdotH, 12.0f) * 0.5f;

    // 色つき金属ハイライト
    float3 metalSpecColor = lerp(float3(0.8f, 0.75f, 0.65f),
                                 visibleBase * 1.5f + Material.Specular.xyz * 0.3f,
                                 0.4f);
    float3 specular = metalSpecColor * (sharpSpec + broadSpec)
                    * Light.Diffuse.xyz;

    // フレネル：控えめに端だけ光る
    float NdotV = max(dot(N, V), 0.0f);
    float fresnel = pow(1.0f - NdotV, 4.0f) * 0.6f;
    float3 fresnelGlow = metalSpecColor * fresnel * Light.Diffuse.xyz;

    // リムライト：輪郭を薄く強調
    float rim = pow(1.0f - NdotV, 5.0f) * max(dot(N, -L), 0.0f) * 0.4f;
    float3 rimLight = metalSpecColor * rim;

    // --- エミッシブ（目・関節の発光） ---
    float3 emissive = Material.Emission.xyz;

    // --- シャドウ ---
    float shadowFactor = CalcShadowFactor(In.ShadowPos);

    float3 finalColor = (diffuse + specular + fresnelGlow + rimLight) * shadowFactor + emissive;

    // --- ヒットフラッシュ ---
    // 被弾した瞬間だけモデル全体を白へ寄せる（FlashIntensity は毎フレーム減衰する）
    finalColor = lerp(finalColor, float3(1.0f, 1.0f, 1.0f), saturate(FlashIntensity));

    outDiffuse = float4(finalColor, baseColor.a);
}
