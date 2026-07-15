#include "straightShotStrategy.h"
#include "weaponData.h"
#include "bulletPool.h"
#include <cstdlib>

// -------------------------------------------------------
// Fire : 弾を1発 Spawn する（spreadAngle 分のランダム拡散あり）
// -------------------------------------------------------
void StraightShotStrategy::Fire(const Vector3& origin, const Vector3& direction,
                                const WeaponData& data, BulletPool& pool)
{
    const float spreadRad = data.spreadAngle * (3.14159265f / 180.0f);

    // 小さなランダム拡散を加える（連射時の揺れ感）
    float randX = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float randY = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

    Vector3 right = { -direction.z, 0.0f, direction.x };
    right.normalize();
    Vector3 up = { 0.0f, 1.0f, 0.0f };

    Vector3 spread = direction
        + right * (randX * spreadRad)
        + up    * (randY * spreadRad);
    spread.normalize();

    Vector3 vel = spread * data.bulletSpeed;
    pool.Spawn(origin, vel, data.bulletLifeTime, data.damage);
}
