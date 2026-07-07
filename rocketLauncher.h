#pragma once
#include "weapon.h"

// =====================================================
// RocketLauncher : ロケットランチャー（Weapon の派生クラス）
// =====================================================
// 弾速が遅く弾倉が少ないが、着弾時に爆風範囲内の全敵に
// 同量のダメージを与えるスプラッシュ攻撃が特徴。
// 爆風半径は GameConfig::RocketLauncher::SPLASH_RADIUS で管理する。
class RocketLauncher : public Weapon
{
protected:
    void Fire(const Vector3& origin, const Vector3& direction) override;

public:
    void Init(BulletPool* pool) override;
    void Uninit() override {}

    const char* GetName() const override { return "ROCKET LAUNCHER"; }
};
