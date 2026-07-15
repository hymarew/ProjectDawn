#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "worldItem.h"
#include "camera.h"
#include "GameConfig.h"
#include <cmath>

ID3D11VertexShader* WorldItem::m_VertexShader = nullptr;
ID3D11PixelShader*  WorldItem::m_PixelShader  = nullptr;
ID3D11InputLayout*  WorldItem::m_VertexLayout = nullptr;
bool                WorldItem::m_IsLoaded     = false;

void WorldItem::ReleaseShaders()
{
    if (m_VertexShader) { m_VertexShader->Release(); m_VertexShader = nullptr; }
    if (m_PixelShader)  { m_PixelShader->Release();  m_PixelShader  = nullptr; }
    if (m_VertexLayout) { m_VertexLayout->Release(); m_VertexLayout = nullptr; }
    m_IsLoaded = false;
}

void WorldItem::Init()
{
    m_Layer    = 1;
    m_IsActive = false;  // プールで事前確保されるため、Activate まで非アクティブ

    // モデルは Activate 時に ItemData の modelPath で差し替える
    // （ModelRenderer は static プールでキャッシュするため Load の呼び直しは軽い）
    AddComponent<ModelRenderer>(this);

    if (!m_IsLoaded)
    {
        Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
            "shader\\ShadowMapLightingVS.cso");
        Renderer::CreatePixelShader(&m_PixelShader,
            "shader\\ShadowMapLightingPS.cso");
        m_IsLoaded = true;
    }

    XMVECTOR dir = XMVector3Normalize(XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f));
    XMStoreFloat4(&Light.Direction, dir);
    Light.Enable  = TRUE;
    Light.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    Light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
}

void WorldItem::Uninit()
{
    GameObject::Uninit();
}

// ---------------------------------------------------------
// Activate : ItemData の内容で表示を設定して起動する（ItemFactory から呼ばれる）
// ---------------------------------------------------------
void WorldItem::Activate(const ItemData& data, const Vector3& pos)
{
    m_ItemID   = data.id;
    m_ItemType = data.type;
    m_Rarity   = data.rarity;

    m_IsActive = true;
    m_BobTimer = 0.0f;

    m_Position = pos;
    m_BaseY    = pos.y + GameConfig::WorldItem::HOVER_HEIGHT;
    m_Rotation = {};
    m_Scale    = { GameConfig::WorldItem::MODEL_SCALE,
                   GameConfig::WorldItem::MODEL_SCALE,
                   GameConfig::WorldItem::MODEL_SCALE };

    const char* modelPath = data.modelPath.empty()
        ? "asset\\model\\crystal.obj" : data.modelPath.c_str();
    GetComponent<ModelRenderer>()->Load(modelPath);

    // レアリティ色をディフューズ光へ反映して発光風に見せる
    RarityColor color = GetRarityColor(m_Rarity);
    Light.Diffuse = XMFLOAT4(color.r, color.g, color.b, 1.0f);
    Light.Ambient = XMFLOAT4(color.r * 0.35f, color.g * 0.35f, color.b * 0.35f, 1.0f);
}

// ---------------------------------------------------------
// Update : 浮遊アニメーション
// 時間経過では消えない（取得 or シーン終了のプールリセットでのみ消える）。
// 取得判定は ItemPickupSystem 側で行う
// ---------------------------------------------------------
void WorldItem::Update(float dt)
{
    if (!m_IsActive) return;
    GameObject::Update(dt);

    // 上下にゆっくり浮遊 + 水平回転
    m_BobTimer += dt;
    m_Position.y  = m_BaseY
                  + sinf(m_BobTimer * GameConfig::WorldItem::BOB_SPEED)
                  * GameConfig::WorldItem::BOB_HEIGHT;
    m_Rotation.y += GameConfig::WorldItem::ROTATE_SPEED * dt;
}

void WorldItem::Draw()
{
    if (!m_IsActive) return;

    Camera* camera = Manager::GetCamera();
    if (camera && !camera->CheckInView(m_Position)) return;

    auto* ctx = Renderer::GetDeviceContext();
    ctx->IASetInputLayout(m_VertexLayout);
    Light.CastShadow = g_CastShadow;
    Renderer::SetLight(Light);
    ctx->VSSetShader(m_VertexShader, nullptr, 0);
    ctx->PSSetShader(m_PixelShader,  nullptr, 0);

    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z)
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    Renderer::SetWorldMatrix(world);
    GameObject::Draw();
}

void WorldItem::DrawShadow()
{
    if (!m_IsActive) return;

    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z)
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    Renderer::SetWorldMatrix(world);
    GameObject::DrawShadow();
}
