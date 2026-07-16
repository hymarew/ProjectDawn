#pragma once
#include "transform.h"
#include "riftMaterial.h"
#include <d3d11.h>

// =====================================================
// RiftRenderer : SpaceRiftの描画のみを担当する（ロジックを持たない）
//
// 本体（ガラスのような裂け目、背景歪み＋発光）と地面投影（発光のみ）の
// 2種類のドローを提供する。テクスチャ・色・時間などのパラメータは
// RiftMaterial から、位置・回転はTransformから受け取るだけで、
// 自身は「シェーダーをセットしてDrawを発行する」以外の責務を持たない。
// =====================================================
class RiftRenderer
{
public:
    void Init();
    void Uninit();

    // 本体描画: Alpha Clip・背景歪み・Emissive・Rim・UVアニメを行う（Alphaブレンド、MRT出力）
    // backgroundSRV: DistortionPassがキャプチャした背景（歪みサンプリング用）
    void Draw(const Transform& transform, const RiftMaterial& material,
              ID3D11ShaderResourceView* crackMaskSRV,
              ID3D11ShaderResourceView* backgroundSRV) const;

    // 地面投影描画: Emissionのみ（Additiveブレンド、MRT出力。Alphaは使わない）
    void DrawGround(const Transform& transform, const RiftMaterial& material,
                    ID3D11ShaderResourceView* crackMaskSRV) const;

private:
    // 本体・地面投影で共通のドロー処理。差分（ピクセルシェーダー・テクスチャ・ブレンド）だけを受け取る
    void DrawInternal(const Transform& transform, const RiftMaterial& material,
                      ID3D11PixelShader* pixelShader,
                      ID3D11ShaderResourceView* const* srvs, UINT srvCount,
                      bool additiveBlend) const;

    ID3D11Buffer*          m_VertexBuffer = nullptr;
    ID3D11InputLayout*     m_VertexLayout = nullptr;
    ID3D11VertexShader*    m_VertexShader = nullptr; // 既存のSpotLightingVSを流用（WorldPosition/Normalを埋める汎用VS）
    ID3D11PixelShader*     m_RiftPS       = nullptr;
    ID3D11PixelShader*     m_GroundPS     = nullptr;
    ID3D11RasterizerState* m_NoCullState  = nullptr; // 板ポリゴンを両面から見せるためのカリング無効ステート
};
