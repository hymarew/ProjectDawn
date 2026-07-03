#ifndef COMMON_FANC_HLSL
#define COMMON_FANC_HLSL

#include "common.hlsl"

// シャドウマップ用のテクスチャとサンプラ
Texture2D g_ShadowMap : register(t1);
SamplerState g_ShadowSampler : register(s1);

// ==========================================================
// 影の濃さ（シャドウファクター）を計算する関数
// 引数 shadowPos : 頂点シェーダーから渡された「ライト視点での座標」
// 戻り値 : 1.0f(光が当たっている) または 0.5f(影になっている)
// ==========================================================
float CalcShadowFactor(float4 shadowPos)
{
    // 追加: 影フラグがOFFなら計算せず常に明るい状態 (1.0f) を返す
    // ※ common.hlslなどで定義されている現在のライト構造体(Light等)を参照してください
    if (!Light.CastShadow) return 1.0f;
    // 初期値は 1.0f (影なし＝元の明るさのまま) とする
    float shadowFactor = 1.0f;

    // 1. 同次座標から正規化デバイス座標(NDC)への変換 (w除算)
    // 遠近法（パースペクティブ）を適用し、XYZの値を -1.0 ～ 1.0 の範囲に収める
    float3 projCoords = shadowPos.xyz / shadowPos.w;

    // 2. シャドウマップ（テクスチャ）から色を読み込むためのUV座標を計算
    float2 shadowUV;
    
    // X座標：NDCの -1.0 ～ 1.0 を、テクスチャUVの 0.0 ～ 1.0 に変換
    shadowUV.x = projCoords.x * 0.5f + 0.5f;
    
    // Y座標：NDCの -1.0 ～ 1.0 を、テクスチャUVの 0.0 ～ 1.0 に変換
    // ※ 画面のY軸（上方向が＋）と、テクスチャのV軸（下方向が＋）は逆なので、マイナスを掛けて反転させる
    shadowUV.y = -projCoords.y * 0.5f + 0.5f;

    // 3. ピクセルが「ライトのカメラが映している範囲（UVが0～1の範囲）」に入っているかチェック
    // 範囲外の場所（ライトの画角の外）に変な影が落ちるのを防ぐ
    if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f &&
        shadowUV.y >= 0.0f && shadowUV.y <= 1.0f)
    {
        // シャドウマップから「ライトから見て一番手前にある物体の深度(Z値)」を取得
        // ※ .r は赤チャンネル(Red)の意味だが、深度マップではZ値がそこに入っている
        float shadowDepth =
            g_ShadowMap.Sample(g_ShadowSampler, shadowUV).r;

        // シャドウアクネ（自分自身の影が縞模様のように落ちるバグ）を防ぐための微小なズレ（バイアス）
        float bias = 0.005f;

        // 4. 深度の比較（影判定）
        // 今描画しようとしているピクセルの深度 (projCoords.z) からバイアスを引いた値が、
        // シャドウマップに記録された一番手前の物体の深度 (shadowDepth) よりも大きい（深い）場合
        // ＝ ライトと自分との間に別の遮蔽物がある ＝ 影になっている！
        if ((projCoords.z - bias) > shadowDepth)
        {
            // 影になっているので、明るさを半分（0.5）にする
            shadowFactor = 0.5f;
        }
    }

    // 最終的な影の濃さを返す
    return shadowFactor;
}

// ==========================================================
// クリスタル向けスペキュラー計算（キラキラ表現）
// 広ハイライト＋鋭いハイライト＋フレネル効果の3層構造
// 引数:
//   N          : 正規化済み法線ベクトル（ワールド空間）
//   L          : 正規化済みライト方向（ライトへ向かう向き = -Light.Direction）
//   V          : 正規化済み視点方向（ワールド座標からカメラへの向き）
//   specColor  : マテリアルのスペキュラー色（Material.Specular.xyz など）
// 戻り値       : 加算するスペキュラー輝度（float3）
// ==========================================================
float3 CalcCrystalSpecular(float3 N, float3 L, float3 V, float3 specColor)
{
    float3 R = reflect(-L, N);
    float RdotV = max(dot(R, V), 0.0f);

    // 広めのハイライト：クリスタル全体がぼんやり輝く
    float specBroad = pow(RdotV, 16.0f) * 3.0f;

    // 鋭いハイライト：面の一部が強く光る
    float specSharp = pow(RdotV, 128.0f) * 8.0f;

    // フレネル効果：端・斜めから見た面ほど強く反射する
    float fresnel = pow(1.0f - max(dot(N, V), 0.0f), 3.0f) * 2.0f;

    return specColor * (specBroad + specSharp) + fresnel;
}

#endif