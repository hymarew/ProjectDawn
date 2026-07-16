#include "main.h"
#include "riftMaterial.h"
#include "renderer.h"

namespace
{
    // shader\riftParams.hlsl の RiftParams (register b7) とレイアウトを一致させること
    struct RiftMaterialCB
    {
        float Time               = 0.0f;
        float GlowStrength       = 0.0f;
        float DistortionStrength = 0.0f;
        float RimPower           = 0.0f;

        float RimIntensity = 0.0f;
        float UVSpeed       = 0.0f;
        float PulseSpeed     = 0.0f;
        float BloomIntensity = 0.0f;

        XMFLOAT4 CenterColor{};
        XMFLOAT4 MidColor{};
        XMFLOAT4 OuterColor{};
        XMFLOAT4 RimColorCyan{};
        XMFLOAT4 RimColorPurple{};

        XMFLOAT2 ScreenSize{};
        XMFLOAT2 _Pad{};
    };
}

void RiftMaterial::Init()
{
    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DEFAULT;
    bd.ByteWidth      = sizeof(RiftMaterialCB);
    bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_Buffer);
}

void RiftMaterial::Uninit()
{
    if (m_Buffer) { m_Buffer->Release(); m_Buffer = nullptr; }
}

void RiftMaterial::Bind() const
{
    RiftMaterialCB cb{};
    cb.Time               = Time;
    cb.GlowStrength       = GlowStrength;
    cb.DistortionStrength = DistortionStrength;
    cb.RimPower           = RimPower;
    cb.RimIntensity       = RimIntensity;
    cb.UVSpeed            = UVSpeed;
    cb.PulseSpeed         = PulseSpeed;
    cb.BloomIntensity     = BloomIntensity;
    cb.CenterColor        = CenterColor;
    cb.MidColor           = MidColor;
    cb.OuterColor         = OuterColor;
    cb.RimColorCyan       = RimColorCyan;
    cb.RimColorPurple     = RimColorPurple;
    cb.ScreenSize         = { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };

    Renderer::GetDeviceContext()->UpdateSubresource(m_Buffer, 0, nullptr, &cb, 0, 0);
    Renderer::GetDeviceContext()->PSSetConstantBuffers(7, 1, &m_Buffer);
}
