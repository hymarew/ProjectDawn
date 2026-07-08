#pragma once
#include "transitionBase.h"
#include "fullScreenQuad.h"
#include "transitionParamsBuffer.h"
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// CircleTransition : 指定した中心点から円形に開閉する演出（アイリス）
//
// 使用例（プレイヤー位置を中心に閉じる、等の演出にも流用できる）:
//   g_TransitionManager.GetTransition<CircleTransition>(TransitionType::Circle)
//       ->SetCenter(0.5f, 0.5f); // 画面中央（デフォルト）
//   g_TransitionManager.Play(TransitionType::Circle, TransitionMode::Out);
// =====================================================
class CircleTransition : public TransitionBase
{
public:
    CircleTransition() : TransitionBase(0.6f, EasingType::InOutCubic) {}

    void Init()   override;
    void Uninit() override;
    void Draw()   override;

    // center : UV座標（0〜1, デフォルトは画面中央）
    void SetCenter(float u, float v) { m_CenterU = u; m_CenterV = v; }
    void SetColor(const XMFLOAT4& color) { m_Color = color; }

private:
    FullScreenQuad         m_Quad;
    TransitionParamsBuffer m_Params;
    ID3D11PixelShader*     m_PixelShader = nullptr;

    float    m_CenterU = 0.5f;
    float    m_CenterV = 0.5f;
    float    m_Feather = 0.03f;
    XMFLOAT4 m_Color   = { 0.0f, 0.0f, 0.0f, 1.0f };
};
