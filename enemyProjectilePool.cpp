#include "main.h"
#include "enemyProjectilePool.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "camera.h"
#include "manager.h"
#include "player.h"
#include "GameConfig.h"

EnemyProjectilePool g_EnemyProjectilePool;

ID3D11VertexShader* EnemyProjectilePool::m_VertexShader = nullptr;
ID3D11PixelShader*  EnemyProjectilePool::m_PixelShader  = nullptr;
ID3D11InputLayout*  EnemyProjectilePool::m_VertexLayout = nullptr;
bool                EnemyProjectilePool::m_IsLoaded     = false;

static const char* PROJECTILE_MODEL_PATH = "asset\\model\\ScorpionNeedle.obj";

// =====================================================
// 弾種別内部パラメータ
// =====================================================
float EnemyProjectilePool::GetRadius(EnemyProjectileType type)
{
    switch (type)
    {
    case EnemyProjectileType::Needle: return GameConfig::EnemyProjectile::NEEDLE_RADIUS;
    default:                          return 0.5f;
    }
}

float EnemyProjectilePool::GetLifeTime(EnemyProjectileType type)
{
    switch (type)
    {
    case EnemyProjectileType::Needle: return GameConfig::EnemyProjectile::NEEDLE_LIFE;
    default:                          return 3.0f;
    }
}

float EnemyProjectilePool::GetScale(EnemyProjectileType type)
{
    switch (type)
    {
    case EnemyProjectileType::Needle: return 5.0f;
    default:                          return 1.0f;
    }
}

// =====================================================
// Init / Uninit
// =====================================================
void EnemyProjectilePool::Init()
{
    m_Pool.resize(GameConfig::EnemyProjectile::POOL_SIZE);

    if (!m_IsLoaded)
    {
        ModelRenderer::Preload(PROJECTILE_MODEL_PATH);
        Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
            "shader\\ShadowMapLightingVS.cso");
        Renderer::CreatePixelShader(&m_PixelShader,
            "shader\\ScorpionPS.cso");
        m_IsLoaded = true;
    }
}

void EnemyProjectilePool::Uninit()
{
    m_Pool.clear();
    if (m_VertexShader) { m_VertexShader->Release(); m_VertexShader = nullptr; }
    if (m_PixelShader)  { m_PixelShader->Release();  m_PixelShader  = nullptr; }
    if (m_VertexLayout) { m_VertexLayout->Release();  m_VertexLayout = nullptr; }
    m_IsLoaded = false;
}

// =====================================================
// Spawn : 空きスロットに弾を生成する
// =====================================================
void EnemyProjectilePool::Spawn(
    EnemyProjectileType type,
    const Vector3&      position,
    const Vector3&      direction,
    float               speed,
    float               damage)
{
    for (auto& p : m_Pool)
    {
        if (p.isActive) continue;
        p.position = position;
        p.velocity = direction * speed;
        p.damage   = damage;
        p.lifeTime = GetLifeTime(type);
        p.radius   = GetRadius(type);
        p.type     = type;
        p.isActive = true;
        return;
    }
}

// =====================================================
// Update : 移動 / 寿命 / プレイヤー当たり判定
// =====================================================
void EnemyProjectilePool::Update(float dt, Player* player)
{
    const float playerR = GameConfig::Collision::PLAYER_RADIUS;

    for (auto& p : m_Pool)
    {
        if (!p.isActive) continue;

        p.position += p.velocity * dt;
        p.lifeTime -= dt;

        if (p.lifeTime <= 0.0f)
        {
            p.isActive = false;
            continue;
        }

        if (player && player->IsAlive())
        {
            float hitDistSq = (playerR + p.radius) * (playerR + p.radius);
            Vector3 diff = player->GetPosition() - p.position;
            if (diff.LengthSq() < hitDistSq)
            {
                player->TakeDamage(p.damage, "EnemyProjectile");
                p.isActive = false;
            }
        }
    }
}

// =====================================================
// Draw : 弾種ごとのスケールでモデル描画
// =====================================================
void EnemyProjectilePool::Draw()
{
    MODEL* model = ModelRenderer::GetCachedModel(PROJECTILE_MODEL_PATH);
    if (!model) return;

    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader,  nullptr, 0);

    Camera* camera = Manager::GetCamera();

    for (auto& p : m_Pool)
    {
        if (!p.isActive) continue;
        if (camera && !camera->CheckInView(p.position)) continue;

        float s = GetScale(p.type);

        XMMATRIX rot = XMMatrixIdentity();
        float len = sqrtf(p.velocity.x * p.velocity.x
                        + p.velocity.y * p.velocity.y
                        + p.velocity.z * p.velocity.z);
        if (len > 0.001f)
        {
            float dx = p.velocity.x / len;
            float dy = p.velocity.y / len;
            float dz = p.velocity.z / len;
            float yaw   = atan2f(dx, dz);
            float pitch = -asinf(dy);
            rot = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);
        }

        XMMATRIX world = XMMatrixScaling(s, s, s)
                       * rot
                       * XMMatrixTranslation(p.position.x, p.position.y, p.position.z);
        Renderer::SetWorldMatrix(world);

        UINT stride = sizeof(VERTEX_3D);
        UINT offset = 0;
        for (unsigned int i = 0; i < model->SubsetNum; i++)
        {
            Renderer::SetMaterial(model->SubsetArray[i].Material.Material);
            if (model->SubsetArray[i].Material.Texture)
                Renderer::GetDeviceContext()->PSSetShaderResources(
                    0, 1, &model->SubsetArray[i].Material.Texture);
            Renderer::GetDeviceContext()->IASetVertexBuffers(
                0, 1, &model->VertexBuffer, &stride, &offset);
            Renderer::GetDeviceContext()->IASetIndexBuffer(
                model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            Renderer::GetDeviceContext()->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Renderer::GetDeviceContext()->DrawIndexed(
                model->SubsetArray[i].IndexNum,
                model->SubsetArray[i].StartIndex, 0);
        }
    }
}

int EnemyProjectilePool::GetActiveCount() const
{
    int cnt = 0;
    for (const auto& p : m_Pool) if (p.isActive) ++cnt;
    return cnt;
}
