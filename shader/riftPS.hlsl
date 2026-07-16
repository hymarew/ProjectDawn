// riftPS.hlsl : SpaceRift（空間の裂け目）本体のピクセルシェーダー
//
// CrackMask（Crack_Mask.png）をAlpha ClipとEmissive両方に使用する。
// 黒部分は完全に透明（discard）、白い部分だけガラスのような裂け目として表示する。
// 背景（DistortionPassがキャプチャしたバックバッファ）を手続き型ノイズで歪ませて
// サンプリングすることで、「裂け目周辺だけ背景が屈折する」表現を1回のDrawで実現する。
//
// SV_Target0 = 通常の合成色（Alphaブレンドでシーンへ重なる）
// SV_Target1 = 発光成分のみ（BloomPassがぼかして加算する）
#include "common.hlsl"
#include "riftParams.hlsl"

Texture2D    g_CrackMask    : register(t0);
Texture2D    g_Background   : register(t1);
SamplerState g_SamplerState : register(s0);

// ---- 手続き型ノイズ（テクスチャ資産を使わないための簡易Value Noise） ----
// distortionPS.hlsl と同じ実装（NoiseTexture資産が無い場合の代替として要件が許容している）
float Hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453123f);
}

float Noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = Hash(i);
    float b = Hash(i + float2(1.0f, 0.0f));
    float c = Hash(i + float2(0.0f, 1.0f));
    float d = Hash(i + float2(1.0f, 1.0f));

    float2 u = f * f * (3.0f - 2.0f * f);
    return lerp(a, b, u.x) + (c - a) * u.y * (1.0f - u.x) + (d - b) * u.x * u.y;
}

void main(in PS_IN In, out float4 outColor : SV_Target0, out float4 outEmissive : SV_Target1)
{
    // ---- UVスクロール ----
    // 完全に流れるのではなく、脈動している程度に少しだけ動かす
    float2 uv = In.TexCoord;
    uv.x += Time * UVSpeed + sin(Time * PulseSpeed) * 0.01f;

    // ---- CrackMaskサンプリング(黒=透明, 白=表示) ----
    float mask = RiftMaskIntensity(g_CrackMask.Sample(g_SamplerState, uv).rgb);

    if (mask < RIFT_ALPHA_CLIP_THRESHOLD)
        discard;

    // ---- 背景の屈折(裂け目の範囲だけ歪む) ----
    float2 screenUV = In.Position.xy / ScreenSize;
    float2 noiseUV  = uv * 3.0f + Time * 0.1f;
    float n1 = Noise(noiseUV) - 0.5f;
    float n2 = Noise(noiseUV + float2(5.2f, 1.3f)) - 0.5f;
    float2 distortedUV = saturate(screenUV + float2(n1, n2) * DistortionStrength);
    float3 backgroundColor = g_Background.Sample(g_SamplerState, distortedUV).rgb;

    // ---- 中心(白)→中間(紫)→外周(青)のグラデーション ----
    float2 centerOffset = uv - 0.5f;
    float dist = saturate(length(centerOffset) * 2.0f);
    float3 glow = lerp(CenterColor.rgb, MidColor.rgb, saturate(dist * 2.0f));
    glow = lerp(glow, OuterColor.rgb, saturate(dist * 2.0f - 1.0f));

    // ---- 明滅(sin(time)で完全に消えない程度に揺らす) ----
    glow *= RiftPulse() * GlowStrength;

    // ---- リムライト(ガラスの縁ほど強くCyan/Purpleに光る) ----
    float3 N = normalize(In.Normal.xyz);
    float3 V = normalize(CameraPosition.xyz - In.WorldPosition.xyz);
    float rim = pow(1.0f - saturate(dot(N, V)), RimPower);
    float3 rimColor = lerp(RimColorCyan.rgb, RimColorPurple.rgb, saturate(dist));
    float3 rimGlow = rimColor * rim * RimIntensity;

    // ---- 最終合成(ガラス越しに背景が透けつつ発光が重なる) ----
    float3 finalColor = lerp(backgroundColor, glow, mask * 0.6f) + rimGlow * mask;

    outColor.rgb = finalColor;
    outColor.a   = mask;

    // ---- Bloom用: 発光成分のみ書き込む ----
    outEmissive.rgb = (glow + rimGlow) * mask * BloomIntensity;
    outEmissive.a   = mask;
}
