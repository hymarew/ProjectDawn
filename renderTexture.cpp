#include "main.h"
#include "renderTexture.h"
#include "renderer.h"

void RenderTexture::Init(DXGI_FORMAT format)
{
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width            = SCREEN_WIDTH;
    desc.Height           = SCREEN_HEIGHT;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = format;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;
    desc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    Renderer::GetDevice()->CreateTexture2D(&desc, nullptr, &m_Texture);
    Renderer::GetDevice()->CreateRenderTargetView(m_Texture, nullptr, &m_RTV);
    Renderer::GetDevice()->CreateShaderResourceView(m_Texture, nullptr, &m_SRV);
}

void RenderTexture::Uninit()
{
    if (m_SRV)     { m_SRV->Release();     m_SRV     = nullptr; }
    if (m_RTV)     { m_RTV->Release();     m_RTV     = nullptr; }
    if (m_Texture) { m_Texture->Release(); m_Texture = nullptr; }
}

void RenderTexture::SetAsRenderTarget() const
{
    Renderer::SetRenderTarget(m_RTV);
}
