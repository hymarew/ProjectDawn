#include "Common.hlsl"

// 風アニメーション用定数バッファ（register b7）
cbuffer WindBuffer : register(b7)
{
    float WindTime;
    float WindStrength;
    float2 WindDummy;
}

void main(in VS_IN In, out PS_IN Out)
{
    // ワールド行列の平行移動成分からツリーの根元座標を取得
    // → ツリーごとに異なる位相になり、全部が同じタイミングで揺れるのを防ぐ
    float3 treeBase = float3(World[3][0], World[3][1], World[3][2]);
    float  phase    = treeBase.x * 0.7f + treeBase.z * 0.5f;

    // ワールド座標に変換
    float4 worldPos = mul(In.Position, World);

    // 根元からの高さ（根元は揺れず、高いほど大きく揺れる）
    float height = max(0.0f, worldPos.y - treeBase.y);

    // サイン波を2本重ねてより自然な揺れに
    float swing = WindStrength * height
                * (sin(WindTime * 2.0f + phase)        * 0.7f
                 + sin(WindTime * 3.3f + phase * 1.4f) * 0.3f);

    worldPos.x += swing;
    worldPos.z += swing * 0.4f;

    Out.WorldPosition = worldPos;

    // ビュー・プロジェクション変換（ワールド座標から再計算）
    Out.Position  = mul(worldPos, View);
    Out.Position  = mul(Out.Position, Projection);

    // シャドウマップ座標
    Out.ShadowPos = mul(worldPos, LightViewProjection);

    // 法線（揺れは位置のみ。法線は元の変換を使用）
    float4 normal = float4(In.Normal.xyz, 0.0f);
    Out.Normal    = normalize(mul(normal, World));

    Out.Diffuse  = In.Diffuse;
    Out.TexCoord = In.TexCoord;
}