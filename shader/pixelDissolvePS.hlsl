// Pixel Dissolve : ドット状のノイズパターンでランダムに覆っていく演出（SFC/PS1期のディザ溶暗風）
// Progress = 0〜1（呼び出し側でIn/Outを考慮済みの進行度）
// Param1   = セルサイズ（ピクセル）。大きいほど粗いドットになる
// Param2   = 境界のぼかし幅
#include "common.hlsl"
#include "transitionCommon.hlsl"

float Hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453123f);
}

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float2 uv = In.TexCoord;

    float cellSize = max(Param1, 1.0f);
    float2 cell    = floor(uv * ScreenSize / cellSize);
    float  n       = Hash(cell); // このセル固有のランダムなしきい値(0〜1)

    float feather = max(Param2, 0.0001f);

    // しきい値をフェザー分だけ内側([feather, 1-feather])に収める。
    // これをしないと n が 0 や 1 に近いセルは Progress=0/1 でも完全に 0/1 にならず、
    // 「覆いきったのに元の画面が透けて見える」「戻りきったのに斑点が残る」原因になる。
    n = lerp(feather, 1.0f - feather, n);

    // Progress が n を超えたセルから順にドット状に覆われていく
    float alpha = smoothstep(n - feather, n + feather, Progress);

    outDiffuse   = Color;
    outDiffuse.a *= alpha;

    if (outDiffuse.a < 0.01f) discard;
}
