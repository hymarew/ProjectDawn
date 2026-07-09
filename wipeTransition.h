#pragma once
#include "transitionBase.h"
#include "fullScreenQuad.h"
#include "transitionParamsBuffer.h"
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// WipeTransition : 指定した角度の直線が画面を横切って覆う/覆いを解く演出
//
// 使用例:
//   g_TransitionManager.GetTransition<WipeTransition>(TransitionType::Wipe)
//       ->SetAngle(XM_PIDIV2); // 下から上へ
//   g_TransitionManager.Play(TransitionType::Wipe, TransitionMode::Out);
// =====================================================
class WipeTransition : public TransitionBase
{
public:
    WipeTransition() : TransitionBase(0.6f, EasingType::InOutCubic) {}

    void Init()   override;
    void Uninit() override;
    void Draw()   override;

    // angle : ワイプ方向（ラジアン）。0=左→右, PI/2=下→上, PI=右→左, -PI/2=上→下
    void SetAngle(float radians) { m_Angle = radians; }
    void SetColor(const XMFLOAT4& color) { m_Color = color; }

private:
    FullScreenQuad        m_Quad;
    TransitionParamsBuffer m_Params;
    ID3D11PixelShader*    m_PixelShader = nullptr;

    float    m_Angle   = 0.0f;                          // ワイプ方向（ラジアン）
    float    m_Feather = 0.06f;                          // 境界のぼかし幅
    XMFLOAT4 m_Color   = { 0.0f, 0.0f, 0.0f, 1.0f };      // 塗りつぶし色
};
