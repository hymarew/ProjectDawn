#pragma once
#include "transitionBase.h"
#include "fullScreenQuad.h"
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// FadeTransition : 単色フェード（黒フェード/白フェード等）
//
// TransitionBase が progress(0→1) とイージングを計算してくれるので、
// ここでは「progress と mode から alpha を求めて塗りつぶす」ことだけを書く。
// =====================================================
class FadeTransition : public TransitionBase
{
public:
    FadeTransition() : TransitionBase(0.5f, EasingType::Linear) {}

    void Init()   override;
    void Uninit() override;
    void Draw()   override;

    // 黒フェード以外（白フェード等）にも流用できるよう色を差し替え可能にしておく
    void SetColor(const XMFLOAT4& color) { m_Color = color; }

private:
    FullScreenQuad m_Quad;
    XMFLOAT4       m_Color = { 0.0f, 0.0f, 0.0f, 1.0f }; // デフォルトは黒
};
