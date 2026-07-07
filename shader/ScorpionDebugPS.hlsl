#include "common.hlsl"

// 法線を色として出力するデバッグシェーダー
// 正常: 上向き面=緑、右向き=赤、前向き=青 のグラデーション
// 異常: 全面が同じ灰色 → 法線が壊れている
void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float3 N = normalize(In.Normal.xyz);
    // -1~1 を 0~1 にリマップして色として表示
    outDiffuse = float4(N * 0.5f + 0.5f, 1.0f);
}
