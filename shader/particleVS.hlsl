#include "common.hlsl"

// ===================================================
// particleVS.hlsl
// パーティクルのインスタンシング描画用頂点シェーダー
//
// 【仕組み】
//   スロット0: 全パーティクルが共有する1枚の板ポリ（4頂点）
//   スロット1: パーティクル1個ごとのインスタンスデータ（位置・サイズ・色・回転）
//   DrawInstanced 1回で数万枚の板ポリをまとめて描画できる。
//
//   旧実装でCPUが毎パーティクル行っていたワールド行列の組み立て
//   （スケール × スピン回転 × ビルボード回転 × 平行移動）を
//   このシェーダー内で行うことで、ドローコールと定数バッファ更新を削減する。
// ===================================================

struct VS_IN_PARTICLE
{
    // ---- スロット0: 共有板ポリ（1頂点ごと） ----
    float3 Position : POSITION0;  // ローカル座標（±0.5 の正方形）
    float2 TexCoord : TEXCOORD0;

    // ---- スロット1: インスタンスデータ（1パーティクルごと） ----
    float4 PosSize  : TEXCOORD1;  // xyz = ワールド位置, w = 現在サイズ
    float4 Color    : TEXCOORD2;  // 現在色（アルファ含む）
    float4 RotFlags : TEXCOORD3;  // x = 面内回転角(rad), y = GroundAligned(0/1), zw = 未使用
};

struct PS_IN_PARTICLE
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

void main(in VS_IN_PARTICLE In, out PS_IN_PARTICLE Out)
{
    float size = In.PosSize.w;
    float rot  = In.RotFlags.x;
    float c    = cos(rot);
    float s    = sin(rot);

    // サイズを適用したローカル座標
    float2 local = In.Position.xy * size;

    float3 offset;
    if (In.RotFlags.y > 0.5f)
    {
        // ---- 地面水平（爆風リング用） ----
        // XY平面の板をXZ平面へ寝かせ、Y軸回転でスピンさせる。
        // CPU実装の RotX(90°) × RotY(rot) と同じ結果になる。
        offset = float3(local.x * c + local.y * s,
                        0.0f,
                       -local.x * s + local.y * c);
    }
    else
    {
        // ---- ビルボード ----
        // 面内スピン（CPU実装の XMMatrixRotationZ と同じ回転方向）
        float2 spun = float2(local.x * c - local.y * s,
                             local.x * s + local.y * c);

        // View行列の回転部分の転置 = ビュー回転の逆行列。
        // これを掛けると板ポリが常にカメラの方を向く。
        float3x3 invViewRot = transpose((float3x3)View);
        offset = mul(float3(spun, 0.0f), invViewRot);
    }

    // ワールド位置 = パーティクル中心 + 回転済みオフセット
    float4 worldPos = float4(In.PosSize.xyz + offset, 1.0f);

    Out.Position = mul(mul(worldPos, View), Projection);
    Out.Color    = In.Color;
    Out.TexCoord = In.TexCoord;
}
