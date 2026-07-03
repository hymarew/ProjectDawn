#pragma once
#include "vector3.h"
#include <d3d11.h>
#include "gameObject.h"

class Field :public GameObject
{
private:

    ID3D11Buffer* m_VertexBuffer;

    ID3D11InputLayout* m_VertexLayout;
    ID3D11VertexShader* m_VertexShader;
    ID3D11PixelShader* m_PixelShader;

    ID3D11ShaderResourceView* m_Texture;

public:
    void Init()override;
    void Uninit()override;
    void Update(float dt) override; 
    void Draw()override;
    void DrawShadow() override;
    const char* GetName() override { return "Field"; }

};