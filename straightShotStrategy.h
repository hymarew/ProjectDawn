#pragma once
#include "attackStrategy.h"

// =====================================================
// StraightShotStrategy : 直進弾を1発撃つ（Assault Rifle 系）
//
// WeaponData::spreadAngle に応じたわずかなランダム拡散を加える。
// 旧 AssaultRifle::Fire() の処理をデータ駆動化したもの。
// =====================================================
class StraightShotStrategy : public IAttackStrategy
{
public:
    void Fire(const Vector3& origin, const Vector3& direction,
              const WeaponData& data, BulletPool& pool) override;
};
