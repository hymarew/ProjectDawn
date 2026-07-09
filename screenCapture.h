#pragma once
#include <d3d11.h>

// =====================================================
// ScreenCapture : 現在のバックバッファの内容をテクスチャへコピーする共通ヘルパー
//
// Mosaic/Blur/Distortion等、「実際の画面」を加工するTransitionが
// 共通で利用する。Capture() を呼んだ時点の画面をシェーダーからサンプリング
// できるようにする（ポストプロセスの入力テクスチャを作る役割）。
// =====================================================
class ScreenCapture
{
public:
    void Init();
    void Uninit();

    // その時点のバックバッファ内容をキャプチャする
    void Capture();

    ID3D11ShaderResourceView* GetSRV() const { return m_SRV; }

private:
    ID3D11Texture2D*          m_Texture = nullptr;
    ID3D11ShaderResourceView* m_SRV     = nullptr;
};
