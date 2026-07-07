#pragma once
#include "gameObject.h"
#include <d3d11.h>

class Grass : public GameObject
{
private:
    ID3D11Buffer*             m_VertexBuffer = nullptr;
    ID3D11ShaderResourceView* m_Texture      = nullptr;

    static ID3D11InputLayout*  m_VertexLayout;
    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader*  m_PixelShader;
    static bool                m_IsLoaded;

public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;
    void DrawShadow()     override {}
    const char* GetName() override { return "Grass"; }

    static void ReleaseShaders();
};
