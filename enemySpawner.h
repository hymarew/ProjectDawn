#pragma once
#include "gameObject.h"
#include "GameConfig.h"
#include "renderer.h"

class EnemyPool;

class EnemySpawner : public GameObject
{
private:
    EnemyPool*  m_pEnemyPool = nullptr;
    GameObject* m_pTarget    = nullptr;

    float m_SpawnTimer    = 0.0f;
    float m_SpawnRate     = 0.0f;

    int   m_BurstCount    = 0;
    float m_IntervalTimer = 0.0f;

    float m_Hp          = 0.0f;
    bool  m_IsDestroyed = false;

    static ID3D11InputLayout*  m_VertexLayout;
    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader*  m_PixelShader;

    LIGHT Light;

public:
    EnemySpawner();
    ~EnemySpawner();

    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;
    void DrawShadow()     override;
    const char* GetName() override { return "EnemySpawner"; }

    float* GetSpawnRatePtr()     { return &m_SpawnRate; }
    void   SetSpawnRate(float r) { m_SpawnRate = r; }
    float  GetHp()    const      { return m_Hp; }
    float  GetMaxHp() const      { return GameConfig::Spawner::MAX_HP; }
    bool   IsDestroyed() const   { return m_IsDestroyed; }

    void TakeDamage(float dmg);

    void SetPoolAndTarget(EnemyPool* pool, GameObject* target)
    {
        m_pEnemyPool = pool;
        m_pTarget    = target;
    }
};
