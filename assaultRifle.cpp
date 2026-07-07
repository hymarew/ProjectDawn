#include "assaultRifle.h"
#include "bulletPool.h"
#include <cmath>

// -------------------------------------------------------
// Init : アサルトライフルのパラメータ設定
// -------------------------------------------------------
void AssaultRifle::Init(BulletPool* pool)
{
    m_BulletPool = pool;

    // ---- 武器挙動パラメータ ----
    m_WeaponParams.magazineSize = 120;   // 120発弾倉
    m_WeaponParams.fireRate     = 0.08f; // 0.08秒に1発（約12連射/秒）
    m_WeaponParams.reloadTime   = 2.5f;  // リロード2.5秒
    m_WeaponParams.zoomRatio    = 1.5f;  // ADS で1.5倍ズーム

    // ---- 弾の性能パラメータ ----
    m_BulletParams.damage        = 40.0f; // 1発40ダメージ
    m_BulletParams.speed         = 70.0f; // 高弾速
    m_BulletParams.lifeTime      = 2.0f;  // 射程はピストルと同じ
    m_BulletParams.spreadAngle   = 2.0f;  // わずかな拡散（連射時の揺れ感）
    m_BulletParams.bulletPerShot = 1;     // 1発射で弾1発

    m_CurrentAmmo = m_WeaponParams.magazineSize;
}

// -------------------------------------------------------
// Fire : 弾を1発 Spawn する（わずかな拡散あり）
// -------------------------------------------------------
void AssaultRifle::Fire(const Vector3& origin, const Vector3& direction)
{
    const float spreadRad = m_BulletParams.spreadAngle * (3.14159265f / 180.0f);

    // 小さなランダム拡散を加える
    float randX = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float randY = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

    Vector3 right = { -direction.z, 0.0f, direction.x };
    right.normalize();
    Vector3 up = { 0.0f, 1.0f, 0.0f };

    Vector3 spread = direction
        + right * (randX * spreadRad)
        + up    * (randY * spreadRad);
    spread.normalize();

    Vector3 vel = spread * m_BulletParams.speed;
    m_BulletPool->Spawn(origin, vel, m_BulletParams.lifeTime, m_BulletParams.damage);
}
