#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// TransitionParamsCB : 独自シェーダーを使う Transition が共通で使う定数バッファのCPU側ミラー
// shader\transitionCommon.hlsl の TransitionParams (register b8) とレイアウトを一致させること。
//
// Progress/Param1~3 の意味は各シェーダー側で自由に決めてよい（例: Wipeなら角度とフェザー幅）。
// =====================================================
struct TransitionParamsCB
{
    float    Progress    = 0.0f;
    float    Param1      = 0.0f;
    float    Param2      = 0.0f;
    float    Param3      = 0.0f;
    XMFLOAT4 Color        = { 0.0f, 0.0f, 0.0f, 1.0f };
    XMFLOAT2 ScreenSize   = { 0.0f, 0.0f };
    XMFLOAT2 _Pad         = { 0.0f, 0.0f };
};

// =====================================================
// TransitionParamsBuffer : 上記CBufferのGPUリソースを管理する小さなヘルパー
// Wipe/Circle/Mosaic等、独自ピクセルシェーダーを使うTransitionから共通で利用する。
// =====================================================
class TransitionParamsBuffer
{
public:
    void Init();
    void Uninit();

    void Update(const TransitionParamsCB& data) const;
    void Bind() const; // register(b8) にバインドする（VS/PS両方）

private:
    ID3D11Buffer* m_Buffer = nullptr;
};
