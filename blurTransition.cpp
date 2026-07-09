#include "main.h"
#include "blurTransition.h"
#include "renderer.h"

void BlurTransition::Init()
{
    m_Quad.Init();
    m_Params.Init();
    m_Capture.Init();
    m_Intermediate.Init();
    Renderer::CreatePixelShader(&m_PixelShader, "shader\\blurPS.cso");
}

void BlurTransition::Uninit()
{
    m_Quad.Uninit();
    m_Params.Uninit();
    m_Capture.Uninit();
    m_Intermediate.Uninit();
    if (m_PixelShader) { m_PixelShader->Release(); m_PixelShader = nullptr; }
}

void BlurTransition::Draw()
{
    m_Capture.Capture();

    float progress  = GetEasedProgress();
    float effective = (GetMode() == TransitionMode::Out) ? progress : (1.0f - progress);
    float radius    = m_MaxRadius * effective;

    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);

    // ---- パス1: 水平ぼかし（ScreenCapture → Intermediate） ----
    {
        TransitionParamsCB cb;
        cb.Progress   = effective;
        cb.Param1     = 1.0f; // dir.x
        cb.Param2     = 0.0f; // dir.y
        cb.Param3     = radius;
        cb.Color      = m_Color;
        cb.ScreenSize = { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };
        m_Params.Update(cb);
        m_Params.Bind();

        ID3D11ShaderResourceView* srv = m_Capture.GetSRV();
        Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &srv);

        m_Intermediate.SetAsRenderTarget();
        m_Quad.DrawGeometry();
    }

    // ---- パス2: 垂直ぼかし（Intermediate → 実際のバックバッファ） ----
    {
        TransitionParamsCB cb;
        cb.Progress   = effective;
        cb.Param1     = 0.0f; // dir.x
        cb.Param2     = 1.0f; // dir.y
        cb.Param3     = radius;
        cb.Color      = m_Color;
        cb.ScreenSize = { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };
        m_Params.Update(cb);
        m_Params.Bind();

        // 先にレンダーターゲットを戻してから SRV をバインドする。
        // 逆順だと Intermediate が「RTVとSRVの同時バインド」というハザードになり、
        // ドライバに強制的に null SRV 扱いにされて真っ黒になってしまう。
        Renderer::RestoreMainRenderTarget();

        ID3D11ShaderResourceView* srv = m_Intermediate.GetSRV();
        Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &srv);

        m_Quad.DrawGeometry();
    }

    // 次に別のシェーダーがt0を使う際に前フレームの中間バッファを誤って読まないようクリアしておく
    ID3D11ShaderResourceView* nullSRV = nullptr;
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &nullSRV);
}
