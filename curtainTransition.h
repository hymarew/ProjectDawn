#pragma once
#include "transitionBase.h"
#include "fullScreenQuad.h"
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// CurtainTransition : 左右2枚の幕が中央で閉じる/中央から開く演出
//
// FullScreenQuad の矩形をワールド変換で半分の幅に縮小＋平行移動させ、
// 左右2枚のパネルとして描画する（独自ジオメトリ・シェーダーは不要）。
// =====================================================
class CurtainTransition : public TransitionBase
{
public:
    CurtainTransition() : TransitionBase(0.6f, EasingType::InOutCubic) {}

    void Init()   override;
    void Uninit() override;
    void Draw()   override;

    void SetColor(const XMFLOAT4& color) { m_Color = color; }

private:
    FullScreenQuad m_Quad;
    XMFLOAT4       m_Color = { 0.0f, 0.0f, 0.0f, 1.0f };
};
