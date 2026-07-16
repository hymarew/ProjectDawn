#pragma once
#include "renderTexture.h"
#include "fullScreenQuad.h"
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// BloomPass : 発光(Emissive)成分をぼかしてバックバッファへ加算合成する汎用ポストプロセス
//
// このプロジェクトのバックバッファはLDR(R8G8B8A8_UNORM)直接描画で、
// 真のHDRレンダーターゲット/輝度抽出パイプラインは持たない。
// そこで「発光させたいオブジェクトだけ」が専用のHDR相当(R16G16B16A16_FLOAT)の
// Emissiveバッファへも同時出力するMRT(複数レンダーターゲット)方式を取る。
// 抽出パスが不要になる分、責務は「ぼかし→加算合成」に絞られる。
//
// 【使い方】
//   BeginEmissivePass();   // 発光オブジェクトの描画をこの間に行う
//   何か発光オブジェクトのDraw()...（メインRTV + EmissiveRTVへ同時出力される）
//   EndEmissivePass();     // 通常のメインRTVへ戻す
//   Composite();           // Emissiveをぼかしてバックバッファへ加算合成
//
// SpaceRift専用ではなく、今後の発光オブジェクト全般が使い回せるよう独立させている。
// =====================================================
class BloomPass
{
public:
    void Init();
    void Uninit();

    void BeginEmissivePass();
    void EndEmissivePass();

    // Emissiveバッファをぼかしてメインバックバッファへ加算合成する
    void Composite();

    void SetBlurRadius(float v) { m_BlurRadius = v; }
    void SetIntensity(float v)  { m_Intensity  = v; }
    float GetBlurRadius() const { return m_BlurRadius; }
    float GetIntensity()  const { return m_Intensity; }

private:
    RenderTexture m_EmissiveTex; // MRTの2枚目（発光成分。HDR相当のフォーマット）
    RenderTexture m_BlurH;       // 水平ブラー結果
    RenderTexture m_BlurV;       // 垂直ブラー結果

    FullScreenQuad     m_Quad;
    ID3D11Buffer*      m_ParamsBuffer = nullptr; // BloomParams (register b9)
    ID3D11PixelShader* m_BlurPS       = nullptr;
    ID3D11PixelShader* m_CompositePS = nullptr;

    float m_BlurRadius = 4.0f; // ぼかし半径（ピクセル）
    float m_Intensity  = 1.0f; // Composite時にEmissiveへ掛ける強さ

    void UpdateParams(const XMFLOAT2& direction) const;
};
