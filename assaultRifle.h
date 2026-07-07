#pragma once
#include "weapon.h"

// =====================================================
// AssaultRifle : アサルトライフル（Weapon の派生クラス）
// =====================================================
// 高い連射速度と大きな弾倉を持つ自動小銃。
// 精度はピストルより低いが、弾数で制圧する戦い方に向いている。
class AssaultRifle : public Weapon
{
protected:
    // 弾を1発 Spawn する。TryFire から呼ばれる
    void Fire(const Vector3& origin, const Vector3& direction) override;

public:
    void Init(BulletPool* pool) override;
    void Uninit() override {}

    const char* GetName() const override { return "ASSAULT RIFLE"; }
};
