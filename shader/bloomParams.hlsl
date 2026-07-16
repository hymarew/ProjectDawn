#ifndef BLOOM_PARAMS_HLSL
#define BLOOM_PARAMS_HLSL

// BloomPass専用の定数バッファ。Transition用のTransitionParams(b8)とは責務を分離する。
cbuffer BloomParams : register(b9)
{
    float2 Direction;   // ブラー方向（水平パスは(1,0)、垂直パスは(0,1)）
    float  BlurRadius;  // ぼかし半径（ピクセル）
    float  Intensity;   // Composite時にEmissiveへ掛ける強さ
    float2 ScreenSize;  // 画面サイズ（ピクセル）
    float2 _Pad;
}

#endif
