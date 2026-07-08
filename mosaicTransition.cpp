#include "main.h"
#include "mosaicTransition.h"
#include "renderer.h"

void MosaicTransition::Init()
{
    m_Quad.Init();
    m_Params.Init();
    m_Capture.Init();
    Renderer::CreatePixelShader(&m_PixelShader, "shader\\mosaicPS.cso");
}

void MosaicTransition::Uninit()
{
    m_Quad.Uninit();
    m_Params.Uninit();
    m_Capture.Uninit();
    if (m_PixelShader) { m_PixelShader->Release(); m_PixelShader = nullptr; }
}

void MosaicTransition::Draw()
{
    // 今描画されている画面をキャプチャしてから加工する（ライブの画面を粗くする）
    m_Capture.Capture();

    float progress = GetEasedProgress();
    float effective = (GetMode() == TransitionMode::Out) ? progress : (1.0f - progress);

    TransitionParamsCB cb;
    cb.Progress   = effective;
    cb.Param1     = m_MaxBlockSize;
    cb.Color      = m_Color;
    cb.ScreenSize = { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };
    m_Params.Update(cb);
    m_Params.Bind();

    ID3D11ShaderResourceView* srv = m_Capture.GetSRV();
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &srv);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);
    m_Quad.DrawGeometry();
}
