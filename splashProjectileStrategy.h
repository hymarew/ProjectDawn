#pragma once
#include "attackStrategy.h"

// =====================================================
// SplashProjectileStrategy : 爆風付きの遅い弾を撃つ（Rocket Launcher 系）
//
// WeaponData::splashRadius / knockbackPower を BulletPool へ渡し、
// 着弾時に範囲ダメージ＋ノックバックを発生させる。
// 発射時はロケット用マズルフラッシュ＋煙を再生する。
// 旧 RocketLauncher::Fire() の処理をデータ駆動化したもの。
// =====================================================
class SplashProjectileStrategy : public IAttackStrategy
{
public:
    void Fire(const Vector3& origin, const Vector3& direction,
              const WeaponData& data, BulletPool& pool) override;
};
