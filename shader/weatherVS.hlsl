#include "common.hlsl"
#include "weatherParams.hlsl"

// ===================================================
// weatherVS.hlsl
// 天候パーティクル（雨・雪）のインスタンシング描画用頂点シェーダー
//
// particleVS.hlsl との違い:
//   - サイズが1つのfloatではなく Scale.xy（横幅×縦の長さ）
//     → 雨の「細長い板ポリ」を表現できる
//   - カメラからの距離を出力し、PSで距離フェードに使う
//   - UVを縦方向へ伸ばせる（雨のスジ表現）
// ===================================================

struct VS_IN_WEATHER
{
    // ---- スロット0: 共有板ポリ（1頂点ごと） ----
    float3 Position : POSITION0;  // ローカル座標（±0.5 の正方形）
    float2 TexCoord : TEXCOORD0;

    // ---- スロット1: インスタンスデータ（1粒ごと） ----
    float4 PosRot   : TEXCOORD1;  // xyz = ワールド位置, w = 面内回転角(rad)
    float4 Scale    : TEXCOORD2;  // xy = 横幅・縦の長さ, zw = 未使用
    float4 Color    : TEXCOORD3;  // 色（アルファ含む）
};

struct PS_IN_WEATHER
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
    float  ViewDist : TEXCOORD1;  // カメラからの距離（距離フェード用）
};

void main(in VS_IN_WEATHER In, out PS_IN_WEATHER Out)
{
    float rot = In.PosRot.w;
    float c   = cos(rot);
    float s   = sin(rot);

    // 横幅・縦の長さを別々に適用したローカル座標（雨は縦長になる）
    float2 local = In.Position.xy * In.Scale.xy;

    // 面内回転（雨は風による傾き、雪は自転）
    float2 spun = float2(local.x * c - local.y * s,
                         local.x * s + local.y * c);

    // ビルボード: View行列の回転部分の転置（=逆行列）で常にカメラの方を向ける
    float3x3 invViewRot = transpose((float3x3)View);
    float3   offset     = mul(float3(spun, 0.0f), invViewRot);

    float4 worldPos = float4(In.PosRot.xyz + offset, 1.0f);
    float4 viewPos  = mul(worldPos, View);

    Out.Position = mul(viewPos, Projection);
    Out.Color    = In.Color;
    Out.ViewDist = length(viewPos.xyz); // カメラからの距離

    // 縦方向へUVを伸ばす（雨のスジ表現。UVStretch=1なら等倍）
    float2 uv = In.TexCoord;
    uv.y = (uv.y - 0.5f) / max(UVStretch, 1.0f) + 0.5f;
    Out.TexCoord = uv;
}
