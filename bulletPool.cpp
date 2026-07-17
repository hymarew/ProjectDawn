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
#include "dynamicLightManager.h"
#include "soundManager.h"

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
    constexpr float kBulletRadius = GameConfig::Bullet::BULLET_RADIUS;

    for (auto& bullet : m_Pool)
    {
        if (!bullet.isActive) continue;

        // 前フレーム位置を保存してから移動する（弾道の線分判定に使う）
        bullet.prevPosition = bullet.position;
        bullet.position    += bullet.velocity * dt;

        // 1フレームの移動量。広域判定の半径に加えることで、
        // 移動線分のどこかが敵のバウンディング球へ届く可能性を漏らさない
        const float travelDist = (bullet.velocity * dt).Length();

        for (auto* e : enemyPool.GetActiveEnemies())
        {
            // 死亡演出中（HP0だがまだ消えていない）の敵は弾が素通りする
            if (e->GetHp() <= 0.0f) continue;

            // ---- 広域判定（バウンディング球）----
            // 大半の弾はここで棄却されるため、マルチスフィア判定の負荷はほぼ増えない
            const float broad = e->GetBroadPhaseRadius() + kBulletRadius + travelDist;
            Vector3 diff = e->GetPosition() - bullet.position;
            if (diff.LengthSq() >= broad * broad) continue;

            // ---- 詳細判定（マルチスフィア × 弾道線分）----
            // 体型に合わせた複数の球で、尻尾・頭にも正しく当たる。
            // 線分判定なので高速弾が細い部位をすり抜けない
            Vector3 hitPos;
            if (e->TestHitSegment(bullet.prevPosition, bullet.position, kBulletRadius, hitPos))
            {
                if (bullet.splashRadius > 0.0f)
                {
                    // ロケット弾: 直撃地点で爆発（範囲ダメージ + 大爆発エフェクト）
                    bullet.position = hitPos; // 爆心地を実際の命中点に合わせる
                    ExplodeSplash(bullet, enemyPool);
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

                    // 通常弾: 硬い装甲に弾かれる被弾演出（火花・装甲片・粉・衝撃リング）。爆発はしない
                    // 発生位置は実際の命中点（尻尾に当たれば尻尾で火花が散る）
                    ParticleManager::GetInstance().EmitScorpionHit(hitPos);
                    bullet.isActive = false;
                }
                break;
            }
        }

        if (!bullet.isActive) continue;

        // スプラッシュ弾（ロケット）は地面に当たっても爆発する（地面撃ちで範囲攻撃ができる）
        if (bullet.splashRadius > 0.0f && bullet.position.y <= 0.0f)
        {
            ExplodeSplash(bullet, enemyPool);
            continue;
        }

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

                    // スポナー破壊時は大爆発演出（発光フラッシュ・火球・火花・デブリ・煙・爆風リング）
                    if (spawner->IsDestroyed())
                        ParticleManager::GetInstance().EmitBigExplosion(spawner->GetPosition());

                    bullet.isActive = false;
                    break;
                }
            }
        }

        if (!bullet.isActive) continue;

        // ---- ロケット弾の飛行演出（噴射炎ライトの追従・火花） ----
        // ※トレイル（発光炎+白煙）は影のちかつきの原因になったため廃止した
        if (bullet.splashRadius > 0.0f)
        {
            if (bullet.lightSlot >= 0)
                g_DynamicLightManager.UpdateSlot(bullet.lightSlot, bullet.position);

            bullet.sparkTimer -= dt;
            if (bullet.sparkTimer <= 0.0f)
            {
                bullet.sparkTimer += GameConfig::RocketFX::SPARK_INTERVAL;
                if (g_RocketSparkEnabled) // デバッグトグル
                    ParticleManager::GetInstance().Emit(ParticlePreset::RocketSpark(), bullet.position);
            }
        }

        bullet.lifeTime -= dt;
        if (bullet.lifeTime <= 0.0f)
        {
            // 何にも当たらず寿命切れした場合も、確保していたライトを解放する
            if (bullet.lightSlot >= 0)
            {
                g_DynamicLightManager.Release(bullet.lightSlot);
                bullet.lightSlot = -1;
            }
            bullet.isActive = false;
        }
    }
}

// =====================================================
// ExplodeSplash : スプラッシュ弾（ロケット）を現在位置で爆発させる
// 敵への直撃時と地面ヒット時の両方から呼ばれる共通処理。
// =====================================================
void BulletPool::ExplodeSplash(Bullet& bullet, EnemyPool& enemyPool)
{
    const float splashR2 = bullet.splashRadius * bullet.splashRadius;

    // 範囲内の敵全員へダメージ + ノックバック
    for (auto* splash : enemyPool.GetActiveEnemies())
    {
        if (splash->GetHp() <= 0.0f) continue; // 死亡演出中はスキップ

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

    // 範囲内のスポナーへダメージ（破壊されたら大爆発）
    for (auto* spawner : m_Spawners)
    {
        if (spawner->IsDestroyed()) continue;
        Vector3 d = spawner->GetPosition() - bullet.position;
        if (d.LengthSq() < splashR2)
        {
            spawner->TakeDamage(bullet.damage);
            g_DamageVisualizer.Add(spawner->GetPosition(), bullet.damage);

            if (spawner->IsDestroyed())
                ParticleManager::GetInstance().EmitBigExplosion(spawner->GetPosition());
        }
    }

    Camera* cam = Manager::GetCamera();
    if (cam) cam->Shake(0.3f, 0.2f);

    // 大爆発演出（発光フラッシュ・火球・火花・デブリ・煙・爆風リング）
    ParticleManager::GetInstance().EmitBigExplosion(bullet.position);
    g_SoundManager.PlaySE(SEType::Explosion1);

    // 噴射炎の追従ライトは役目を終えたので先に解放する（AddFlashがスロット不足で
    // 取りこぼさないよう、爆発フラッシュを追加する前に解放しておく）
    if (bullet.lightSlot >= 0)
    {
        g_DynamicLightManager.Release(bullet.lightSlot);
        bullet.lightSlot = -1;
    }

    // 爆発時の瞬間的な強いポイントライト（急速に減衰し、寿命が尽きたら自動で消える）
    if (g_ExplosionLightEnabled) // デバッグトグル（影ちかつき切り分け用）
    {
        g_DynamicLightManager.AddFlash(
            bullet.position,
            GameConfig::RocketFX::EXPLOSION_LIGHT_RADIUS,
            Vector3(1.0f, 0.6f, 0.2f), // 黄〜オレンジ
            GameConfig::RocketFX::EXPLOSION_LIGHT_INTENSITY,
            GameConfig::RocketFX::EXPLOSION_LIGHT_LIFE);
    }

    bullet.isActive = false;
}

Bullet* BulletPool::Spawn(const Vector3& pos, const Vector3& velocity, float lifeTime, float damage,
                           float splashRadius, float knockbackPower)
{
    for (auto& bullet : m_Pool)
    {
        if (!bullet.isActive)
        {
            bullet.position       = pos;
            bullet.prevPosition   = pos; // 生成フレームの線分判定は長さ0から始める
            bullet.velocity       = velocity;
            bullet.lifeTime       = lifeTime;
            bullet.damage         = damage;
            bullet.splashRadius   = splashRadius;
            bullet.knockbackPower = knockbackPower;
            bullet.isActive       = true;
            bullet.sparkTimer     = 0.0f;

            // ロケット弾は噴射炎の追従ポイントライトを確保する（デバッグトグルでOFF可能）
            bullet.lightSlot = (splashRadius > 0.0f && g_RocketLightEnabled)
                ? g_DynamicLightManager.Acquire(pos,
                      GameConfig::RocketFX::TRAIL_LIGHT_RADIUS,
                      Vector3(1.0f, 0.6f, 0.15f), // オレンジ〜黄橙色
                      GameConfig::RocketFX::TRAIL_LIGHT_INTENSITY)
                : -1;

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
