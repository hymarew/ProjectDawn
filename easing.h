#pragma once
#include <cmath>

// =====================================================
// Easing : 汎用イージング関数ライブラリ
//
// Transition専用にはせず、UIアニメーション・カメラ演出・
// パーティクル・ボス演出など、0→1の進行度を扱うあらゆる場所から
// 利用できるよう独立させている。
//
// 使い方:
//   float t = Easing::OutCubic(progress);       // 関数を直接呼ぶ
//   float t = Easing::Apply(EasingType::OutCubic, progress); // enumで動的に選ぶ
// =====================================================

enum class EasingType
{
    Linear,
    InQuad,  OutQuad,  InOutQuad,
    InCubic, OutCubic, InOutCubic,
    InSine,  OutSine,  InOutSine,
};

namespace Easing
{
    constexpr float HALF_PI = 1.57079632679f;
    constexpr float PI      = 3.14159265359f;

    inline float Linear(float t) { return t; }

    inline float InQuad(float t)  { return t * t; }
    inline float OutQuad(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
    inline float InOutQuad(float t)
    {
        return (t < 0.5f)
            ? 2.0f * t * t
            : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) * 0.5f;
    }

    inline float InCubic(float t)  { return t * t * t; }
    inline float OutCubic(float t) { return 1.0f - powf(1.0f - t, 3.0f); }
    inline float InOutCubic(float t)
    {
        return (t < 0.5f)
            ? 4.0f * t * t * t
            : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) * 0.5f;
    }

    inline float InSine(float t)    { return 1.0f - cosf(t * HALF_PI); }
    inline float OutSine(float t)   { return sinf(t * HALF_PI); }
    inline float InOutSine(float t) { return -(cosf(PI * t) - 1.0f) * 0.5f; }

    // EasingType を指定して適用する（設定データから動的に選びたい場合に使う）
    inline float Apply(EasingType type, float t)
    {
        switch (type)
        {
        case EasingType::Linear:     return Linear(t);
        case EasingType::InQuad:     return InQuad(t);
        case EasingType::OutQuad:    return OutQuad(t);
        case EasingType::InOutQuad:  return InOutQuad(t);
        case EasingType::InCubic:    return InCubic(t);
        case EasingType::OutCubic:   return OutCubic(t);
        case EasingType::InOutCubic: return InOutCubic(t);
        case EasingType::InSine:     return InSine(t);
        case EasingType::OutSine:    return OutSine(t);
        case EasingType::InOutSine:  return InOutSine(t);
        default:                     return Linear(t);
        }
    }
}
