// Wipe : 指定した角度の直線が画面を横切りながら覆っていく/覆いを解いていく演出
// Progress = 0〜1（呼び出し側でIn/Outを考慮済みの進行度）
// Param1   = 角度（ラジアン）。0で左→右
// Param2   = 境界のぼかし幅（0でハードエッジ）
#include "common.hlsl"
#include "transitionCommon.hlsl"

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float2 uv = In.TexCoord;

    float2 dir = float2(cos(Param1), sin(Param1));

    // UV空間([-0.5,0.5]の正方形)を dir 方向へ射影したときの最大値（矩形の角で決まる）
    float maxProj = max(0.5f * (abs(dir.x) + abs(dir.y)), 0.0001f);

    // UVを中心基準にして方向ベクトルへ射影し、0〜1のワイプ座標に正規化する
    float2 centered = uv - 0.5f;
    float  proj     = dot(centered, dir);
    float  coord    = saturate(proj / (2.0f * maxProj) + 0.5f);

    float feather = max(Param2, 0.0001f);
    float alpha   = 1.0f - smoothstep(Progress - feather, Progress + feather, coord);

    outDiffuse   = Color;
    outDiffuse.a *= alpha;

    if (outDiffuse.a < 0.01f) discard;
}
