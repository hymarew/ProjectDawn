#include "main.h"
#include "pixelDissolveTransition.h"
#include "renderer.h"

void PixelDissolveTransition::Init()
{
    m_Quad.Init();
    m_Params.Init();
    Renderer::CreatePixelShader(&m_PixelShader, "shader\\pixelDissolvePS.cso");
}

void PixelDissolveTransition::Uninit()
{
    m_Quad.Uninit();
    m_Params.Uninit();
    if (m_PixelShader) { m_PixelShader->Release(); m_PixelShader = nullptr; }
}

void PixelDissolveTransition::Draw()
{
    float progress  = GetEasedProgress();
    float effective = (GetMode() == TransitionMode::Out) ? progress : (1.0f - progress);

    TransitionParamsCB cb;
    cb.Progress   = effective;
    cb.Param1     = m_CellSize;
    cb.Param2     = m_Feather;
    cb.Color      = m_Color;
    cb.ScreenSize = { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };
    m_Params.Update(cb);
    m_Params.Bind();

    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);
    m_Quad.DrawGeometry();
}
