#pragma once
#include "transitionBase.h"
#include "fullScreenQuad.h"
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// SlideTransition : 単色パネルが画面外からスライドして覆う/覆いを解く演出
//
// Wipe/Circleと違い独自ピクセルシェーダーは使わず、矩形の位置を
// ワールド変換でずらすだけで実現する（FullScreenQuad::Draw の座標オフセット機能を利用）。
// Out: 画面外(direction方向) → 覆う位置(0,0) へスライドイン
// In : 覆う位置(0,0) → 反対側の画面外 へスライドアウト
// =====================================================
class SlideTransition : public TransitionBase
{
public:
    SlideTransition() : TransitionBase(0.5f, EasingType::OutCubic) {}

    void Init()   override;
    void Uninit() override;
    void Draw()   override;

    // direction : スライドする向き（正規化不要。x=1,y=0で右から進入する）
    void SetDirection(float dx, float dy) { m_Direction = { dx, dy }; }
    void SetColor(const XMFLOAT4& color)  { m_Color = color; }

private:
    FullScreenQuad m_Quad;
    XMFLOAT2       m_Direction = { 1.0f, 0.0f }; // デフォルト: 右から進入
    XMFLOAT4       m_Color     = { 0.0f, 0.0f, 0.0f, 1.0f };
};
