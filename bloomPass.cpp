#include "main.h"
#include "bloomPass.h"
#include "renderer.h"

namespace
{
    // shader\bloomParams.hlsl の BloomParams (register b9) とレイアウトを一致させること
    struct BloomParamsCB
    {
        XMFLOAT2 Direction  = { 0.0f, 0.0f };
        float    BlurRadius = 0.0f;
        float    Intensity  = 0.0f;
        XMFLOAT2 ScreenSize = { 0.0f, 0.0f };
        XMFLOAT2 _Pad       = { 0.0f, 0.0f };
    };
}

void BloomPass::Init()
{
    // Emissiveとブラー中間バッファはHDR相当の値(1.0超)を保持したいのでFLOATフォーマットにする
    m_EmissiveTex.Init(DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_BlurH.Init(DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_BlurV.Init(DXGI_FORMAT_R16G16B16A16_FLOAT);

    m_Quad.Init();

    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DEFAULT;
    bd.ByteWidth      = sizeof(BloomParamsCB);
    bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_ParamsBuffer);

    Renderer::CreatePixelShader(&m_BlurPS,      "shader\\bloomBlurPS.cso");
    Renderer::CreatePixelShader(&m_CompositePS, "shader\\bloomCompositePS.cso");
}

void BloomPass::Uninit()
{
    m_EmissiveTex.Uninit();
    m_BlurH.Uninit();
    m_BlurV.Uninit();
    m_Quad.Uninit();

    if (m_ParamsBuffer) { m_ParamsBuffer->Release(); m_ParamsBuffer = nullptr; }
    if (m_BlurPS)       { m_BlurPS->Release();       m_BlurPS       = nullptr; }
    if (m_CompositePS)  { m_CompositePS->Release();  m_CompositePS  = nullptr; }
}

void BloomPass::UpdateParams(const XMFLOAT2& direction) const
{
    BloomParamsCB cb{};
    cb.Direction  = direction;
    cb.BlurRadius = m_BlurRadius;
    cb.Intensity  = m_Intensity;
    cb.ScreenSize = { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };

    Renderer::GetDeviceContext()->UpdateSubresource(m_ParamsBuffer, 0, nullptr, &cb, 0, 0);
    Renderer::GetDeviceContext()->PSSetConstantBuffers(9, 1, &m_ParamsBuffer);
}

// ---------------------------------------------------------
// BeginEmissivePass : メインRTV + EmissiveRTVへ同時出力する(MRT)
// 発光オブジェクトのPixelShaderは SV_Target0(通常色) と SV_Target1(発光成分) の
// 2つを書き出すことで、通常描画と発光抽出を1回のDrawで両立させる
// ---------------------------------------------------------
void BloomPass::BeginEmissivePass()
{
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    Renderer::GetDeviceContext()->ClearRenderTargetView(m_EmissiveTex.GetRTV(), clearColor);

    ID3D11RenderTargetView* rtvs[2] = { Renderer::GetMainRenderTargetView(), m_EmissiveTex.GetRTV() };
    Renderer::GetDeviceContext()->OMSetRenderTargets(2, rtvs, Renderer::GetDepthStencilView());
}

void BloomPass::EndEmissivePass()
{
    Renderer::RestoreMainRenderTarget();
}

// ---------------------------------------------------------
// Composite : Emissiveを水平→垂直にぼかし、加算合成でバックバッファへ焼き込む
// ---------------------------------------------------------
void BloomPass::Composite()
{
    auto* ctx = Renderer::GetDeviceContext();

    // 1. 水平ブラー: EmissiveTex -> BlurH
    m_BlurH.SetAsRenderTarget();
    UpdateParams({ 1.0f, 0.0f });
    ID3D11ShaderResourceView* srv = m_EmissiveTex.GetSRV();
    ctx->PSSetShaderResources(0, 1, &srv);
    ctx->PSSetShader(m_BlurPS, nullptr, 0);
    m_Quad.DrawGeometry();

    // 2. 垂直ブラー: BlurH -> BlurV
    m_BlurV.SetAsRenderTarget();
    UpdateParams({ 0.0f, 1.0f });
    srv = m_BlurH.GetSRV();
    ctx->PSSetShaderResources(0, 1, &srv);
    m_Quad.DrawGeometry();

    // 3. Composite: バックバッファへ加算合成
    Renderer::RestoreMainRenderTarget();
    Renderer::SetAdditiveBlend(true);
    srv = m_BlurV.GetSRV();
    ctx->PSSetShaderResources(0, 1, &srv);
    ctx->PSSetShader(m_CompositePS, nullptr, 0);
    m_Quad.DrawGeometry();
    Renderer::SetAdditiveBlend(false);
}
