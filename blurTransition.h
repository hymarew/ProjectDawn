#pragma once
#include "transitionBase.h"
#include "fullScreenQuad.h"
#include "transitionParamsBuffer.h"
#include "screenCapture.h"
#include "renderTexture.h"
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// BlurTransition : 実際の画面をガウシアン風にぼかしながら覆う演出
//
// 水平パス→垂直パスの2パス構成（separable gaussian blur）。
// 1. 現在の画面を ScreenCapture へキャプチャ
// 2. 水平ぼかし: ScreenCapture を読み、RenderTexture(中間バッファ)へ書く
// 3. 垂直ぼかし: RenderTexture を読み、実際のバックバッファへ書く
// =====================================================
class BlurTransition : public TransitionBase
{
public:
    BlurTransition() : TransitionBase(0.8f, EasingType::InCubic) {}

    void Init()   override;
    void Uninit() override;
    void Draw()   override;

    void SetColor(const XMFLOAT4& color) { m_Color = color; }
    void SetMaxRadius(float pixels)      { m_MaxRadius = pixels; }

private:
    FullScreenQuad         m_Quad;
    TransitionParamsBuffer m_Params;
    ScreenCapture           m_Capture;      // 元画面
    RenderTexture           m_Intermediate; // 水平パスの出力先
    ID3D11PixelShader*     m_PixelShader = nullptr;

    XMFLOAT4 m_Color     = { 0.0f, 0.0f, 0.0f, 1.0f };
    float    m_MaxRadius = 6.0f;
};
