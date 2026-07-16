#ifndef RIFT_PARAMS_HLSL
#define RIFT_PARAMS_HLSL

// SpaceRift専用の定数バッファ。C++側は riftMaterial.cpp の RiftMaterialCB と
// レイアウトを一致させること。
cbuffer RiftParams : register(b7)
{
    float Time;               // 経過時間（秒）
    float GlowStrength;       // 発光全体の強さ（乗算）
    float DistortionStrength; // 背景の屈折の強さ（0.003〜0.01目安）
    float RimPower;           // フレネル指数

    float RimIntensity;  // リムライトの強さ
    float UVSpeed;        // UVスクロール速度
    float PulseSpeed;      // 明滅速度
    float BloomIntensity; // Bloomバッファへ書き込む発光の強さ（HDR値3〜8目安）

    float4 CenterColor;    // 中心色（白）
    float4 MidColor;       // 中間色（紫）
    float4 OuterColor;     // 外周色（青）
    float4 RimColorCyan;   // リム色1（Cyan）
    float4 RimColorPurple; // リム色2（Purple）

    float2 ScreenSize; // 画面サイズ（ピクセル。背景歪みのスクリーンUV計算用）
    float2 _Pad;
}

// ==========================================================
// 本体（riftPS）と地面投影（riftGroundPS）で共通のヘルパー
// ==========================================================

// CrackMaskの黒部分を完全透明として捨てる閾値
static const float RIFT_ALPHA_CLIP_THRESHOLD = 0.04f;

// CrackMask（グレースケール想定）のRGBから表示強度（0=透明, 1=表示）を求める
float RiftMaskIntensity(float3 maskTex)
{
    return dot(maskTex, float3(0.299f, 0.587f, 0.114f));
}

// sin(time)による発光の明滅（完全には消えない0.6〜1.0の範囲で揺らす）
float RiftPulse()
{
    return 0.8f + 0.2f * sin(Time * PulseSpeed);
}

#endif
