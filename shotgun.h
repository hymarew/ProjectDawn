#pragma once
#include "weapon.h"

// =====================================================
// Shotgun : ショットガン（Weapon の派生クラス）
// =====================================================
// 1回の射撃で複数の弾を扇状に発射する。
// 近距離で高威力だが、遠距離では拡散して当たりにくい。
// bulletPerShot の数だけ BulletPool::Spawn を呼ぶ。
class Shotgun : public Weapon
{
protected:
    // 複数弾を扇状に Spawn する。TryFire から呼ばれる
    void Fire(const Vector3& origin, const Vector3& direction) override;

public:
    void Init(BulletPool* pool) override;
    void Uninit() override {}

    const char* GetName() const override { return "Shotgun"; }
};
