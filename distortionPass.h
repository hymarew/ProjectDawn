#pragma once
#include "screenCapture.h"

// =====================================================
// DistortionPass : 画面歪み用の背景キャプチャ提供のみを担当する
//
// 「裂け目の向こう側の景色を歪ませて見せる」ために必要な、
// 歪ませる前の背景テクスチャ（現在のバックバッファ）の提供に責務を絞る。
// 実際のUVオフセット計算（ノイズ生成・歪みの適用）はRiftRendererが使う
// riftPS.hlsl 側で行う（GPU上で1回のサンプリングにまとめるため）。
// =====================================================
class DistortionPass
{
public:
    void Init();
    void Uninit();

    // Rift本体を描画する前に呼ぶ。その時点のバックバッファをキャプチャする
    void Capture();

    ID3D11ShaderResourceView* GetBackgroundSRV() const { return m_Capture.GetSRV(); }

private:
    ScreenCapture m_Capture;
};
