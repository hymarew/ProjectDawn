#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "box.h"
#include "boxCollider.h"
#include "collisionManager.h"
#include "camera.h"
#include "GameConfig.h"

ID3D11VertexShader* Box::m_VertexShader = nullptr;
ID3D11PixelShader*  Box::m_PixelShader  = nullptr;
ID3D11InputLayout*  Box::m_VertexLayout  = nullptr;
bool                Box::m_IsLoaded      = false;

void Box::Init()
{
    m_Layer    = 1;
    m_Position = { 0.0f, 0.0f, 0.0f };

    AddComponent<ModelRenderer>(this)->Load("asset\\model\\box.obj");

    // AABB の半サイズ（モデルのスケールに合わせて調整）
    auto* col = AddComponent<BoxCollider>(this);
    col->Setup(Vector3(GameConfig::Collision::BOX_HALF_SIZE,
                       GameConfig::Collision::BOX_HALF_SIZE,
                       GameConfig::Collision::BOX_HALF_SIZE), ColliderTag::Box);
    g_CollisionManager.Register(col);

    if (!m_IsLoaded)
    {
        Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
            "shader\\ShadowMapLightingVS.cso");
        Renderer::CreatePixelShader(&m_PixelShader,
            "shader\\ShadowMapLightingPS.cso");
        m_IsLoaded = true;
    }
}

void Box::Uninit()
{
    BoxCollider* col = GetComponent<BoxCollider>();
    if (col) g_CollisionManager.Unregister(col);

    GameObject::Uninit();
}

void Box::Update(float dt)
{
    GameObject::Update(dt); // BoxCollider の位置同期
}

void Box::Draw()
{
    Camera* camera = Manager::GetCamera();
    if (!camera->CheckInView(m_Position)) return;

    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    Light.CastShadow = g_CastShadow;
    Renderer::SetLight(Light);

    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot   = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);
    GameObject::Draw();
}

void Box::DrawShadow()
{
    Camera* camera = Manager::GetCamera();
    if (!camera->CheckInView(m_Position, 3.0f)) return;

    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot   = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);
    GameObject::DrawShadow();
}
