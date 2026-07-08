#pragma once
#include "transitionBase.h"
#include "fullScreenQuad.h"
#include "transitionParamsBuffer.h"
#include "screenCapture.h"
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// MosaicTransition : 実際の画面をブロック状に粗くしながら覆う演出
//
// 毎フレーム ScreenCapture で「今描画されている画面」をテクスチャへコピーし、
// mosaicPS.hlsl でブロック化して描き直す。ライブの画面を加工するため、
// Fade等の単色オーバーレイ方式（FullScreenQuad::Draw）は使わない。
// =====================================================
class MosaicTransition : public TransitionBase
{
public:
    MosaicTransition() : TransitionBase(0.8f, EasingType::InCubic) {}

    void Init()   override;
    void Uninit() override;
    void Draw()   override;

    void SetColor(const XMFLOAT4& color) { m_Color = color; }
    void SetMaxBlockSize(float pixels)   { m_MaxBlockSize = pixels; }

private:
    FullScreenQuad         m_Quad;
    TransitionParamsBuffer m_Params;
    ScreenCapture           m_Capture;
    ID3D11PixelShader*     m_PixelShader = nullptr;

    XMFLOAT4 m_Color        = { 0.0f, 0.0f, 0.0f, 1.0f };
    float    m_MaxBlockSize = 48.0f;
};
