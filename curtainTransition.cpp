#include "main.h"
#include "curtainTransition.h"

void CurtainTransition::Init()
{
    m_Quad.Init();
}

void CurtainTransition::Uninit()
{
    m_Quad.Uninit();
}

void CurtainTransition::Draw()
{
    float t = GetEasedProgress();

    // Out: 閉じきっていない(t=0) → 中央で閉じる(t=1)
    // In : 閉じている(t=0) → 左右へ開ききる(t=1)
    float closeAmount = (GetMode() == TransitionMode::Out) ? t : (1.0f - t);

    const float halfWidth = (float)SCREEN_WIDTH * 0.5f;

    // 左パネル: 閉じているとき x=0（左半分を覆う）、開くと -halfWidth（画面外）
    float leftX  = -halfWidth * (1.0f - closeAmount);
    // 右パネル: 閉じているとき x=halfWidth（右半分を覆う）、開くと +SCREEN_WIDTH（画面外）
    float rightX = halfWidth + halfWidth * (1.0f - closeAmount);

    XMMATRIX scaleHalf = XMMatrixScaling(0.5f, 1.0f, 1.0f);

    XMMATRIX leftWorld  = scaleHalf * XMMatrixTranslation(leftX,  0.0f, 0.0f);
    XMMATRIX rightWorld = scaleHalf * XMMatrixTranslation(rightX, 0.0f, 0.0f);

    m_Quad.Draw(m_Color, leftWorld);
    m_Quad.Draw(m_Color, rightWorld);
}
