#pragma once
#include <d3d11.h>

// =====================================================
// RenderTexture : 画面サイズのオフスクリーンレンダーターゲット
//
// Blur等のマルチパスポストプロセスで「中間結果の描画先」として使う。
// RTV(描画先として使う)とSRV(シェーダーで読む)の両方を持つ。
// =====================================================
class RenderTexture
{
public:
    void Init();
    void Uninit();

    // このテクスチャを描画先にする（Draw後は Renderer::RestoreMainRenderTarget() で元に戻すこと）
    void SetAsRenderTarget() const;

    ID3D11ShaderResourceView* GetSRV() const { return m_SRV; }

private:
    ID3D11Texture2D*          m_Texture = nullptr;
    ID3D11RenderTargetView*   m_RTV     = nullptr;
    ID3D11ShaderResourceView* m_SRV     = nullptr;
};
