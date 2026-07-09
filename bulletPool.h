#pragma once
#include <vector>
#include "bullet.h"

class EnemyPool;
class EnemySpawner;

// =====================================================
// BulletPool : 弾のメモリ管理・更新・ヒット判定
// =====================================================
class BulletPool
{
private:
    std::vector<Bullet>        m_Pool;
    int                        m_MaxBullets = 0;
    std::vector<EnemySpawner*> m_Spawners;
    int m_ShotsFired = 0;
    int m_ShotsHit   = 0;

    // スプラッシュ弾（ロケット）を現在位置で爆発させる。
    // 範囲内の敵・スポナーへダメージ/ノックバックを与え、大爆発エフェクトを再生して弾を消す。
    // 敵への直撃時と地面ヒット時の両方から呼ばれる。
    void ExplodeSplash(Bullet& bullet, EnemyPool& enemyPool);

public:
    BulletPool()  {}
    ~BulletPool() {}

    void Init(int maxBullets);
    void Uninit();

    void Update(float dt, EnemyPool& enemyPool);

    void RegisterSpawner(EnemySpawner* spawner) { m_Spawners.push_back(spawner); }

    Bullet* Spawn(const Vector3& pos, const Vector3& velocity, float lifeTime, float damage,
                  float splashRadius = 0.0f, float knockbackPower = 0.0f);

    int GetActiveCount() const;
    int GetMaxBullets()  const { return m_MaxBullets; }
    int GetShotsFired()  const { return m_ShotsFired; }
    int GetShotsHit()    const { return m_ShotsHit; }
    void ResetStats()          { m_ShotsFired = 0; m_ShotsHit = 0; }

    std::vector<Bullet>& GetPool() { return m_Pool; }
};
