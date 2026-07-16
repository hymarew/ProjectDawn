#ifndef WEATHER_PARAMS_HLSL
#define WEATHER_PARAMS_HLSL

// 天候パーティクル専用の定数バッファ。
// C++側は weatherRenderer.cpp の WeatherParamsCB とレイアウトを一致させること。
// 雨・雪のドローコールごとに値を差し替える（雨はフェード無効値を入れる）。
cbuffer WeatherParams : register(b10)
{
    float FadeStart;  // この距離から徐々に透明へ（m）
    float FadeEnd;    // この距離で完全に消える（m）
    float UVStretch;  // 縦方向へUVを伸ばす倍率（雨=Length/Width比に応じた値、雪=1）
    float _Pad;
}

#endif
