#pragma once

#include "vector3.h"
#include <d3d11.h>
#include "gameObject.h"

class Explosion :public GameObject
{
private:
    int m_Frame;


    ID3D11Buffer* m_VertexBuffer;
    static ID3D11InputLayout* m_VertexLayout;
    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader* m_PixelShader;

    static ID3D11ShaderResourceView* m_Texture;

public:
    void Init()override;
    void Uninit()override;
    void Update(float dt) override;
    void Draw()override;
    void DrawShadow() override;
    const char* GetName() override { return "Explosion"; }

};