#include "splashProjectileStrategy.h"
#include "weaponData.h"
#include "bulletPool.h"
#include "particleManager.h"
#include "manager.h"
#include "soundManager.h"

void SplashProjectileStrategy::Fire(const Vector3& origin, const Vector3& direction,
                                    const WeaponData& data, BulletPool& pool)
{
    Vector3 vel = direction * data.bulletSpeed;
    pool.Spawn(origin, vel,
               data.bulletLifeTime,
               data.damage,
               data.splashRadius,
               data.knockbackPower);

    // 発射時マズルフラッシュ（通常武器より一回り大きい）+ 煙（デバッグトグルでOFF可能）
    if (g_RocketMuzzleEnabled)
    {
        auto& particleManager = ParticleManager::GetInstance();
        particleManager.Emit(ParticlePreset::RocketMuzzleFlash(), origin);
        particleManager.Emit(ParticlePreset::RocketMuzzleSmoke(), origin);
    }

    g_SoundManager.PlaySE(SEType::RocketLauncher);
}
