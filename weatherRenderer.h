#pragma once
#include "weatherSetting.h"
#include <d3d11.h>
#include <vector>

// ===================================================
// WeatherRenderer : 天候パーティクルのGPUインスタンシング描画のみを担当
//
// 既存の ParticleRenderer と同じ方式:
//   スロット0 = 全粒共有の板ポリ4頂点 / スロット1 = 粒ごとのインスタンスデータ。
//   インスタンスバッファへ毎フレーム1回 Map(WRITE_DISCARD) で書き込み、
//   雨・雪それぞれ1回ずつの DrawInstanced で描画する（合計2ドローコール）。
//   最大15000粒でもCPU負荷はバッファ転送1回分のみで、60FPS維持に十分軽い。
//
// ※名前について: 要求仕様の「ParticleRenderer」に対応するクラスだが、
//   既存の particleRenderer.h と衝突するため WeatherRenderer とした。
// ===================================================
class WeatherRenderer
{
public:
    // maxInstances: インスタンスバッファの容量（雨と雪の合計最大数。固定確保）
    void Init(int maxInstances);
    void Uninit();

    // 1バッチ（雨または雪）を描画する。
    // instances     : 描画する粒のインスタンス配列
    // fadeStart/End : 距離フェード範囲（フェード不要なら十分大きい値を渡す）
    // uvStretch     : 縦方向へUVを伸ばす倍率（雨のスジ表現。雪は1.0）
    void DrawBatch(const std::vector<WeatherInstance>& instances,
                   float fadeStart, float fadeEnd, float uvStretch);

    int GetDrawCallCount() const { return m_DrawCalls; }
    void ResetStats() { m_DrawCalls = 0; }

private:
    ID3D11Buffer*             m_QuadVertexBuffer = nullptr; // 全粒共有の板ポリ（4頂点）
    ID3D11Buffer*             m_InstanceBuffer   = nullptr; // インスタンスデータ（DYNAMIC）
    ID3D11Buffer*             m_ParamsBuffer     = nullptr; // WeatherParams (register b10)
    ID3D11InputLayout*        m_InputLayout      = nullptr;
    ID3D11VertexShader*       m_VertexShader     = nullptr;
    ID3D11PixelShader*        m_PixelShader      = nullptr;
    ID3D11ShaderResourceView* m_Texture          = nullptr; // 雨・雪共通の丸グラデーション

    int m_MaxInstances = 0;
    int m_DrawCalls    = 0; // 統計（デバッグUI表示用）
};
