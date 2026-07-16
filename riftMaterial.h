#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// RiftMaterial : SpaceRiftの見た目パラメータの保持とGPU定数バッファ更新のみを担当
//
// 「テクスチャ・色・Emission・UV速度などを管理する」という単一責務(SRP)に絞り、
// 時間経過や脈動の計算は RiftController、実際の描画は RiftRenderer が担当する。
// shader\riftParams.hlsl の RiftParams (register b7) とレイアウトを一致させること。
// =====================================================
class RiftMaterial
{
public:
    void Init();
    void Uninit();

    // 現在の値をGPU定数バッファへ書き込み、PSのb7にバインドする
    void Bind() const;

    // ---- RiftController・ImGuiデバッグから編集される値 ----
    float Time               = 0.0f;
    float GlowStrength       = 1.0f;   // 発光全体の強さ（乗算）
    float DistortionStrength = 0.006f; // 背景の屈折の強さ（0.003〜0.01目安）
    float RimPower           = 3.0f;   // フレネル指数

    float RimIntensity  = 1.5f;  // リムライトの強さ
    float UVSpeed        = 0.02f; // UVスクロール速度
    float PulseSpeed      = 1.5f;  // 明滅速度
    float BloomIntensity = 5.0f;  // Bloomバッファへ書き込む発光の強さ（HDR値3〜8目安）

    // 中心(白)→中間(紫)→外周(青)のグラデーション
    XMFLOAT4 CenterColor = { 1.0f,  1.0f,  1.0f,  1.0f };
    XMFLOAT4 MidColor    = { 0.55f, 0.15f, 0.9f,  1.0f };
    XMFLOAT4 OuterColor  = { 0.15f, 0.35f, 1.0f,  1.0f };

    // リムライト2色（Cyan / Purple）
    XMFLOAT4 RimColorCyan   = { 0.3f, 0.9f, 1.0f, 1.0f };
    XMFLOAT4 RimColorPurple = { 0.7f, 0.2f, 1.0f, 1.0f };

private:
    ID3D11Buffer* m_Buffer = nullptr;
};
