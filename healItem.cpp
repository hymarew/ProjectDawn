#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "healItem.h"
#include "sphereCollider.h"
#include "player.h"
#include "particleManager.h"
#include "camera.h"
#include "GameConfig.h"

ID3D11VertexShader* HealItem::m_VertexShader = nullptr;
ID3D11PixelShader*  HealItem::m_PixelShader  = nullptr;
ID3D11InputLayout*  HealItem::m_VertexLayout = nullptr;
bool                HealItem::m_IsLoaded     = false;

void HealItem::ReleaseShaders()
{
    if (m_VertexShader) { m_VertexShader->Release(); m_VertexShader = nullptr; }
    if (m_PixelShader)  { m_PixelShader->Release();  m_PixelShader  = nullptr; }
    if (m_VertexLayout) { m_VertexLayout->Release();  m_VertexLayout = nullptr; }
    m_IsLoaded = false;
}

void HealItem::Init()
{
    m_Layer    = 1;
    m_Consumed = false;
    m_Scale    = { GameConfig::HealItem::MODEL_SCALE,
                   GameConfig::HealItem::MODEL_SCALE,
                   GameConfig::HealItem::MODEL_SCALE };

    AddComponent<ModelRenderer>(this)->Load("asset\\model\\HealBox.obj");

    // 取得判定用コライダー（CollisionManager には登録しない = 物理押し出しを受けない）
    AddComponent<SphereCollider>(this)->Setup(
        GameConfig::HealItem::PICKUP_RADIUS, ColliderTag::Obstacle);

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

void HealItem::Uninit()
{
    GameObject::Uninit();
}

void HealItem::Update(float dt)
{
    GameObject::Update(dt);

    // 多重取得防止: 取得済みなら削除待ちの間も判定しない
    if (m_Consumed) return;

    Player* player = Manager::GetGameObject<Player>();
    if (!player) return;

    // プレイヤーがコライダーへ侵入した瞬間に取得する
    const float pickupDist = GameConfig::HealItem::PICKUP_RADIUS
                           + GameConfig::Collision::PLAYER_RADIUS;
    Vector3 diff = player->GetPosition() - m_Position;
    if (diff.LengthSq() > pickupDist * pickupDist) return;

    m_Consumed = true;

    // 回復量 = 最大HP × 10%（Heal 内で最大HPを超えないよう Clamp される）
    player->Heal(player->GetMaxHp() * GameConfig::HealItem::HEAL_RATIO);

    // 回復エフェクト（プレイヤーの身体を中心に緑の光の粒が舞い上がる）
    Vector3 fxPos = player->GetPosition();
    fxPos.y += 1.0f;
    ParticleManager::GetInstance().EmitHeal(fxPos);

    // アイテムを削除（GameScene の remove_if が今フレーム末に回収する）
    SetDestroy();
}

void HealItem::Draw()
{
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

void HealItem::DrawShadow()
{
    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z)
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    Renderer::SetWorldMatrix(world);
    GameObject::DrawShadow();
}
