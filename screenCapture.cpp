#include "main.h"
#include "screenCapture.h"
#include "renderer.h"

void ScreenCapture::Init()
{
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width            = SCREEN_WIDTH;
    desc.Height           = SCREEN_HEIGHT;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM; // スワップチェーンと同じフォーマット
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;
    desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    Renderer::GetDevice()->CreateTexture2D(&desc, nullptr, &m_Texture);
    Renderer::GetDevice()->CreateShaderResourceView(m_Texture, nullptr, &m_SRV);
}

void ScreenCapture::Uninit()
{
    if (m_SRV)     { m_SRV->Release();     m_SRV     = nullptr; }
    if (m_Texture) { m_Texture->Release(); m_Texture = nullptr; }
}

void ScreenCapture::Capture()
{
    Renderer::CopyBackBufferTo(m_Texture);
}
