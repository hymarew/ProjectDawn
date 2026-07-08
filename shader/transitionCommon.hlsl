#ifndef TRANSITION_COMMON_HLSL
#define TRANSITION_COMMON_HLSL

// Wipe/Circle/Mosaic等、独自ピクセルシェーダーを使うTransitionが共通で使う定数バッファ。
// C++側は transitionParamsBuffer.h の TransitionParamsCB とレイアウトを一致させること。
// Progress/Param1~3 の意味は各シェーダーごとに自由に決めてよい。
cbuffer TransitionParams : register(b8)
{
    float  Progress;    // 0〜1（イージング適用後の進行度）
    float  Param1;
    float  Param2;
    float  Param3;
    float4 Color;       // 塗りつぶし色
    float2 ScreenSize;  // 画面サイズ（ピクセル）
    float2 _Pad;
}

#endif
