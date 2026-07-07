#include "main.h"
#include "enemySpawner.h"
#include "enemyPool.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "sphereCollider.h"
#include "collisionManager.h"
#include "GameConfig.h"
#include <stdlib.h>

ID3D11VertexShader* EnemySpawner::m_VertexShader = nullptr;
ID3D11PixelShader*  EnemySpawner::m_PixelShader  = nullptr;
ID3D11InputLayout*  EnemySpawner::m_VertexLayout  = nullptr;

EnemySpawner::EnemySpawner() : m_pEnemyPool(nullptr), m_pTarget(nullptr), m_SpawnTimer(0)
{
}

EnemySpawner::~EnemySpawner()
{
}

void EnemySpawner::Init()
{
    m_Layer       = 1;
    m_SpawnTimer  = 0;
    m_Hp          = GameConfig::Spawner::MAX_HP;
    m_IsDestroyed = false;

    AddComponent<ModelRenderer>(this)->Load("asset\\model\\crystal.obj");
    m_Scale = { 3.0f, 3.0f, 3.0f };

    XMVECTOR dir = XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f);
    dir = XMVector3Normalize(dir);
    XMStoreFloat4(&Light.Direction, dir);
    Light.Enable  = TRUE;
    Light.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    Light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);

    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
        "shader\\ShadowMapLightingVS.cso");
    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\CrystalPS.cso");

    auto* col = AddComponent<SphereCollider>(this);
    col->Setup(GameConfig::Spawner::COLLIDER_RADIUS, ColliderTag::Spawner);
    g_CollisionManager.Register(col);
}

void EnemySpawner::Uninit()
{
    SphereCollider* col = GetComponent<SphereCollider>();
    if (col) g_CollisionManager.Unregister(col);

    GameObject::Uninit();
}

void EnemySpawner::TakeDamage(float dmg)
{
    if (m_IsDestroyed) return;
    m_Hp -= dmg;
    if (m_Hp <= 0.0f)
    {
        m_Hp          = 0.0f;
        m_IsDestroyed = true;
    }
}

void EnemySpawner::Update(float dt)
{
    if (m_IsDestroyed) return;
    if (m_pEnemyPool == nullptr) return;

    if (m_IntervalTimer > 0.0f)
    {
        m_IntervalTimer -= dt;
        if (m_IntervalTimer < 0.0f) m_IntervalTimer = 0.0f;
        GameObject::Update(dt);
        return;
    }

    m_SpawnTimer += dt;

    float spawnInterval = 1.0f / m_SpawnRate;
    while (m_SpawnTimer >= spawnInterval)
    {
        m_SpawnTimer -= spawnInterval;

        float offsetX = (rand() % 100 - 50) * (GameConfig::Spawner::SPAWN_OFFSET_RANGE / 50.0f);
        float offsetZ = (rand() % 100 - 50) * (GameConfig::Spawner::SPAWN_OFFSET_RANGE / 50.0f);

        Vector3 spawnPos = m_Position;
        spawnPos.x += offsetX;
        spawnPos.z += offsetZ;
        spawnPos.y += GameConfig::Spawner::SPAWN_Y_OFFSET;

        m_pEnemyPool->SpawnEnemy(spawnPos, m_pTarget);
        m_BurstCount++;

        if (m_BurstCount >= GameConfig::Spawner::BURST_COUNT)
        {
            m_BurstCount    = 0;
            m_IntervalTimer = GameConfig::Spawner::BURST_INTERVAL;
            m_SpawnTimer    = 0.0f;
            break;
        }
    }

    GameObject::Update(dt);
}

void EnemySpawner::DrawShadow()
{
    if (m_IsDestroyed) return;

    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z)
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

    Renderer::SetWorldMatrix(world);
    GameObject::DrawShadow();
}

void EnemySpawner::Draw()
{
    if (m_IsDestroyed) return;

    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
    Light.CastShadow = g_CastShadow;
    Renderer::SetLight(Light);
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z)
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

    Renderer::SetWorldMatrix(world);
    GameObject::Draw();
}
