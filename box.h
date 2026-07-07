#pragma once
#include "vector3.h"
#include <d3d11.h>
#include "gameObject.h"
#include "renderer.h"

class Box : public GameObject
{
private:
    static ID3D11InputLayout*  m_VertexLayout;
    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader*  m_PixelShader;
    static bool                m_IsLoaded;

    LIGHT Light;

public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;
    void DrawShadow()     override;
    const char* GetName() override { return "Box"; }
};