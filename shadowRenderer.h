#pragma once
#include "main.h"
#include "renderer.h"

class ShadowRenderer
{
public:

    void Init();
    void Uninit();
    void Update();

    // ============================================
    // ShadowPass
    // ============================================

    void Begin();
    void End();
    void SetShadowMap();

    LIGHT& GetLight() { return m_Light; }
    XMMATRIX GetLightViewProjection() const { return m_LightVP; }

private:

    LIGHT m_Light{};
    XMMATRIX m_LightView{};
    XMMATRIX m_LightProj{};
    XMMATRIX m_LightVP{};

    // ============================================
    // ShadowShader
    // ============================================

    ID3D11PixelShader* m_ShadowPS = nullptr;
    ID3D11VertexShader* m_ShadowVS = nullptr;
    ID3D11InputLayout* m_ShadowLayout = nullptr;
};