#pragma once
#include "transitionBase.h"
#include "fullScreenQuad.h"
#include "transitionParamsBuffer.h"
#include "screenCapture.h"
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// DistortionTransition : 実際の画面を手続き型ノイズでUV歪みさせる演出（熱波/ワープ風）
// ノイズはシェーダー内で生成するため専用テクスチャ資産は不要。
// =====================================================
class DistortionTransition : public TransitionBase
{
public:
    DistortionTransition() : TransitionBase(0.8f, EasingType::Linear) {}

    void Init()   override;
    void Uninit() override;
    void Draw()   override;

    void SetColor(const XMFLOAT4& color) { m_Color = color; }
    void SetStrength(float v) { m_Strength = v; }
    void SetScale(float v)    { m_Scale = v; }

private:
    FullScreenQuad         m_Quad;
    TransitionParamsBuffer m_Params;
    ScreenCapture           m_Capture;
    ID3D11PixelShader*     m_PixelShader = nullptr;

    XMFLOAT4 m_Color    = { 0.0f, 0.0f, 0.0f, 1.0f };
    float    m_Strength = 0.06f; // UVオフセットの最大量
    float    m_Scale    = 6.0f;  // ノイズの細かさ
};
