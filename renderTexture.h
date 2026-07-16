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
    // format : 既定はバックバッファと同じLDRフォーマット。
    // Bloom等、HDR相当の値(1.0超)を保持したい場合は DXGI_FORMAT_R16G16B16A16_FLOAT を指定する
    void Init(DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
    void Uninit();

    // このテクスチャを描画先にする（Draw後は Renderer::RestoreMainRenderTarget() で元に戻すこと）
    void SetAsRenderTarget() const;

    ID3D11ShaderResourceView* GetSRV() const { return m_SRV; }
    ID3D11RenderTargetView*   GetRTV() const { return m_RTV; }

private:
    ID3D11Texture2D*          m_Texture = nullptr;
    ID3D11RenderTargetView*   m_RTV     = nullptr;
    ID3D11ShaderResourceView* m_SRV     = nullptr;
};
