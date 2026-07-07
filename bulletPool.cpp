#include "main.h"
#include "bulletPool.h"
#include "enemyPool.h"
#include "enemy.h"
#include "enemySpawner.h"
#include "manager.h"
#include "camera.h"
#include "explosion.h"
#include "particleManager.h"
#include "damageVisualizer.h"
#include "GameConfig.h"

void BulletPool::Init(int maxBullets)
{
    m_MaxBullets = maxBullets;
    m_Pool.resize(maxBullets);
}

void BulletPool::Uninit()
{
    m_Pool.clear();
    m_Spawners.clear();
}

void BulletPool::Update(float dt, EnemyPool& enemyPool)
{
    constexpr float kHitDist  = GameConfig::Collision::SCORPION_RADIUS
                               + GameConfig::Bullet::BULLET_RADIUS;
    constexpr float kHitDist2 = kHitDist * kHitDist;

    for (auto& bullet : m_Pool)
    {
        if (!bullet.isActive) continue;

        bullet.position += bullet.velocity * dt;

        for (auto* e : enemyPool.GetActiveEnemies())
        {
            Vector3 diff = e->GetPosition() - bullet.position;
            if (diff.LengthSq() < kHitDist2)
            {
                if (bullet.splashRadius > 0.0f)
                {
                    const float splashR2 = bullet.splashRadius * bullet.splashRadius;
                    for (auto* splash : enemyPool.GetActiveEnemies())
                    {
                        Vector3 d = splash->GetPosition() - bullet.position;
                        if (d.LengthSq() < splashR2)
                        {
                            splash->TakeDamage(bullet.damage);
                            g_DamageVisualizer.Add(splash->GetPosition(), bullet.damage);
                            if (bullet.knockbackPower > 0.0f)
                            {
                                float len = d.Length();
                                if (len > 0.01f)
                                    splash->AddKnockback(d * (1.0f / len), bullet.knockbackPower);
                            }
                        }
                    }
                    for (auto* spawner : m_Spawners)
                    {
                        if (spawner->IsDestroyed()) continue;
                        Vector3 d = spawner->GetPosition() - bullet.position;
                        if (d.LengthSq() < splashR2)
                        {
                            spawner->TakeDamage(bullet.damage);
                            g_DamageVisualizer.Add(spawner->GetPosition(), bullet.damage);
                        }
                    }
                    Camera* cam = Manager::GetCamera();
                    if (cam) cam->Shake(0.3f, 0.2f);
                }
                else
                {
                    e->TakeDamage(bullet.damage);
                    m_ShotsHit++;
                    if (e->GetHp() <= 0.0f)
                    {
                        Camera* cam = Manager::GetCamera();
                        if (cam) cam->Shake(0.15f, 0.1f);
                    }
                    g_DamageVisualizer.Add(e->GetPosition(), bullet.damage);
                }

                Manager::AddGameObject<Explosion>()->SetPosition(bullet.position);
                ParticleManager::GetInstance().Emit(EffectType::Spark, bullet.position);

                bullet.isActive = false;
                break;
            }
        }

        if (!bullet.isActive) continue;

        if (bullet.splashRadius <= 0.0f)
        {
            for (auto* spawner : m_Spawners)
            {
                if (spawner->IsDestroyed()) continue;

                constexpr float kSpawnerHitDist  = GameConfig::Spawner::COLLIDER_RADIUS
                                                  + GameConfig::Bullet::BULLET_RADIUS;
                constexpr float kSpawnerHitDist2 = kSpawnerHitDist * kSpawnerHitDist;

                Vector3 d = spawner->GetPosition() - bullet.position;
                if (d.LengthSq() < kSpawnerHitDist2)
                {
                    spawner->TakeDamage(bullet.damage);
                    g_DamageVisualizer.Add(spawner->GetPosition(), bullet.damage);

                    if (spawner->IsDestroyed())
                        Manager::AddGameObject<Explosion>()->SetPosition(spawner->GetPosition());

                    bullet.isActive = false;
                    break;
                }
            }
        }

        if (!bullet.isActive) continue;

        bullet.lifeTime -= dt;
        if (bullet.lifeTime <= 0.0f)
            bullet.isActive = false;
    }
}

Bullet* BulletPool::Spawn(const Vector3& pos, const Vector3& velocity, float lifeTime, float damage,
                           float splashRadius, float knockbackPower)
{
    for (auto& bullet : m_Pool)
    {
        if (!bullet.isActive)
        {
            bullet.position       = pos;
            bullet.velocity       = velocity;
            bullet.lifeTime       = lifeTime;
            bullet.damage         = damage;
            bullet.splashRadius   = splashRadius;
            bullet.knockbackPower = knockbackPower;
            bullet.isActive       = true;
            m_ShotsFired++;
            return &bullet;
        }
    }
    return nullptr;
}

int BulletPool::GetActiveCount() const
{
    int count = 0;
    for (const auto& bullet : m_Pool)
        if (bullet.isActive) count++;
    return count;
}
