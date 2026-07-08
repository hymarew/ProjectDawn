#include "main.h"
#include "slideTransition.h"

void SlideTransition::Init()
{
    m_Quad.Init();
}

void SlideTransition::Uninit()
{
    m_Quad.Uninit();
}

void SlideTransition::Draw()
{
    float t = GetEasedProgress();

    // 画面サイズ分オフセットした「画面外」の位置を求める
    XMFLOAT2 offscreen = { m_Direction.x * SCREEN_WIDTH, m_Direction.y * SCREEN_HEIGHT };
    XMFLOAT2 start, end;

    if (GetMode() == TransitionMode::Out)
    {
        start = offscreen;          // 画面外（見えない）
        end   = { 0.0f, 0.0f };     // 画面を覆う位置
    }
    else
    {
        start = { 0.0f, 0.0f };                          // 覆っている状態
        end   = { -offscreen.x, -offscreen.y };           // 反対側の画面外へ
    }

    float offsetX = start.x + (end.x - start.x) * t;
    float offsetY = start.y + (end.y - start.y) * t;

    XMMATRIX world = XMMatrixTranslation(offsetX, offsetY, 0.0f);
    m_Quad.Draw(m_Color, world);
}
