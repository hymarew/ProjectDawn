#include "main.h"
#include "wipeTransition.h"
#include "renderer.h"

void WipeTransition::Init()
{
    m_Quad.Init();
    m_Params.Init();
    Renderer::CreatePixelShader(&m_PixelShader, "shader\\wipePS.cso");
}

void WipeTransition::Uninit()
{
    m_Quad.Uninit();
    m_Params.Uninit();
    if (m_PixelShader) { m_PixelShader->Release(); m_PixelShader = nullptr; }
}

void WipeTransition::Draw()
{
    float progress = GetEasedProgress();
    // Out(通常→覆う)ならそのまま、In(覆う→通常)なら反転させて共通シェーダーに渡す
    float effective = (GetMode() == TransitionMode::Out) ? progress : (1.0f - progress);

    TransitionParamsCB cb;
    cb.Progress = effective;
    cb.Param1   = m_Angle;
    cb.Param2   = m_Feather;
    cb.Color    = m_Color;
    m_Params.Update(cb);
    m_Params.Bind();

    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);
    m_Quad.DrawGeometry();
}
