#include "rocketLauncher.h"
#include "bulletPool.h"
#include "particleManager.h"
#include "manager.h"
#include "GameConfig.h"

void RocketLauncher::Init(BulletPool* pool)
{
    m_BulletPool = pool;

    m_WeaponParams.magazineSize = GameConfig::RocketLauncher::MAGAZINE_SIZE;
    m_WeaponParams.fireRate     = GameConfig::RocketLauncher::FIRE_RATE;
    m_WeaponParams.reloadTime   = GameConfig::RocketLauncher::RELOAD_TIME;
    m_WeaponParams.zoomRatio    = 1.0f;
    m_WeaponParams.singleShot   = true;

    m_BulletParams.damage        = GameConfig::RocketLauncher::DAMAGE;
    m_BulletParams.speed         = GameConfig::RocketLauncher::SPEED;
    m_BulletParams.lifeTime      = GameConfig::RocketLauncher::LIFE_TIME;
    m_BulletParams.spreadAngle   = 0.0f;
    m_BulletParams.bulletPerShot = 1;

    m_CurrentAmmo = m_WeaponParams.magazineSize;
}

void RocketLauncher::Fire(const Vector3& origin, const Vector3& direction)
{
    Vector3 vel = direction * m_BulletParams.speed;
    m_BulletPool->Spawn(
        origin, vel,
        m_BulletParams.lifeTime,
        m_BulletParams.damage,
        GameConfig::RocketLauncher::SPLASH_RADIUS,
        GameConfig::RocketLauncher::KNOCKBACK_POWER
    );

    // 発射時マズルフラッシュ（通常武器より一回り大きい）+ 煙（デバッグトグルでOFF可能）
    if (g_RocketMuzzleEnabled)
    {
        auto& particleManager = ParticleManager::GetInstance();
        particleManager.Emit(ParticlePreset::RocketMuzzleFlash(), origin);
        particleManager.Emit(ParticlePreset::RocketMuzzleSmoke(), origin);
    }
}
