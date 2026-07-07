#include "shotgun.h"
#include "bulletPool.h"
#include <cmath>

// -------------------------------------------------------
// Init : ショットガンのパラメータ設定
// -------------------------------------------------------
void Shotgun::Init(BulletPool* pool)
{
    m_BulletPool = pool;

    // ---- 武器挙動パラメータ ----
    m_WeaponParams.magazineSize = 6;     // 6発装填
    m_WeaponParams.fireRate     = 0.8f;  // 0.8秒に1発（ポンプアクション想定）
    m_WeaponParams.reloadTime   = 2.0f;  // リロード2秒
    m_WeaponParams.zoomRatio    = 1.0f;

    // ---- 弾の性能パラメータ ----
    m_BulletParams.damage        = 8.0f;  // 1粒あたりのダメージ（全弾命中で48ダメージ）
    m_BulletParams.speed         = 40.0f; // 弾速は遅め
    m_BulletParams.lifeTime      = 0.5f;  // 寿命が短い = 射程が短い
    m_BulletParams.spreadAngle   = 15.0f; // 拡散角 ±15度
    m_BulletParams.bulletPerShot = 6;     // 1射で6粒発射

    m_CurrentAmmo = m_WeaponParams.magazineSize;
}

// -------------------------------------------------------
// Fire : 複数の弾を扇状に Spawn する
// -------------------------------------------------------
// bulletPerShot の数だけ、direction を中心に
// spreadAngle の範囲でランダムにばらけた方向へ弾を発射する。
void Shotgun::Fire(const Vector3& origin, const Vector3& direction)
{
    const float spreadRad = m_BulletParams.spreadAngle * (3.14159265f / 180.0f);

    for (int i = 0; i < m_BulletParams.bulletPerShot; i++)
    {
        // -1.0f 〜 +1.0f のランダム値を生成して拡散角を決める
        float randX = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float randY = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        // direction を中心にして横・縦それぞれ spreadRad の範囲でずらす
        // 右ベクトル（direction に垂直）を求めて拡散方向を作る
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
}
