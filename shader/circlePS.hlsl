// Circle : 画面中心（または任意の点）から円形に開閉する演出（アイリス）
// Progress = 0〜1（呼び出し側でIn/Outを考慮済みの進行度。1で画面全体を覆う半径になる）
// Param1   = 中心X（UV, 0〜1）
// Param2   = 中心Y（UV, 0〜1）
// Param3   = 境界のぼかし幅
#include "common.hlsl"
#include "transitionCommon.hlsl"

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float2 uv     = In.TexCoord;
    float2 center = float2(Param1, Param2);

    // アスペクト比を補正して真円になるようにする（ScreenSizeはピクセル単位）
    float  aspect = ScreenSize.x / max(ScreenSize.y, 1.0f);
    float2 diff   = uv - center;
    diff.x *= aspect;

    // 画面四隅のうち中心から最も遠い点までの距離を最大半径とする
    float2 c0 = float2(0.0f, 0.0f) - center; c0.x *= aspect;
    float2 c1 = float2(1.0f, 0.0f) - center; c1.x *= aspect;
    float2 c2 = float2(0.0f, 1.0f) - center; c2.x *= aspect;
    float2 c3 = float2(1.0f, 1.0f) - center; c3.x *= aspect;
    float maxRadius = max(max(length(c0), length(c1)), max(length(c2), length(c3)));
    maxRadius = max(maxRadius, 0.0001f);

    float dist   = length(diff) / maxRadius; // 0〜1に正規化
    float feather = max(Param3, 0.0001f);

    // 円形の「窓」の半径。Progress=0で全開(1.0)、Progress=1で完全に閉じる(0.0)
    float radiusThreshold = 1.0f - Progress;

    // 窓の外側(dist > radiusThreshold)を覆う
    float alpha = smoothstep(radiusThreshold - feather, radiusThreshold + feather, dist);

    outDiffuse   = Color;
    outDiffuse.a *= alpha;

    if (outDiffuse.a < 0.01f) discard;
}
