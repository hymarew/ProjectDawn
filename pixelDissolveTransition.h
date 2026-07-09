#pragma once
#include "transitionBase.h"
#include "fullScreenQuad.h"
#include "transitionParamsBuffer.h"
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// PixelDissolveTransition : ドット状のノイズパターンでランダムに覆っていく演出
// （SFC/PS1期のディザ溶暗風）。ノイズはシェーダー内で生成するため専用テクスチャは不要。
// =====================================================
class PixelDissolveTransition : public TransitionBase
{
public:
    PixelDissolveTransition() : TransitionBase(0.7f, EasingType::Linear) {}

    void Init()   override;
    void Uninit() override;
    void Draw()   override;

    void SetColor(const XMFLOAT4& color) { m_Color = color; }
    void SetCellSize(float pixels) { m_CellSize = pixels; }

private:
    FullScreenQuad         m_Quad;
    TransitionParamsBuffer m_Params;
    ID3D11PixelShader*     m_PixelShader = nullptr;

    XMFLOAT4 m_Color    = { 0.0f, 0.0f, 0.0f, 1.0f };
    float    m_CellSize = 6.0f;
    float    m_Feather  = 0.08f;
};
