#pragma once
#include "vector3.h"
#include <d3d11.h>
#include "gameObject.h"
#include "renderer.h"

class Player : public GameObject
{
private:
    Vector3 m_Velocity{ 0.0f,0.0f,0.0f };

    ID3D11InputLayout* m_VertexLayout;
    ID3D11VertexShader* m_VertexShader;
    ID3D11PixelShader* m_PixelShader;

    LIGHT m_Light;
    bool m_Ground = true;
    float m_MoveAnimation = 0.0f;

public:
    void Init()    override;
    void Uninit()  override;
    void Update(float dt) override;
    void Draw()    override;
    void DrawShadow() override;
    const char* GetName() override { return "Player"; }
};
