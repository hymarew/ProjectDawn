#pragma once
#include "weapon.h"

// =====================================================
// Pistol : ピストル（Weapon の派生クラス）
// =====================================================
// 単発・高精度・中程度の連射速度を持つ基本的な銃。
// 武器システムの動作確認として最初に実装した武器で、
// 以前 Player が直接行っていた射撃処理を移植したもの。
//
// 新しい武器を追加するときは、この Pistol を参考に
// Weapon を継承して Init と Fire を実装すればよい。
class Pistol : public Weapon
{
protected:
    // 実際に弾を BulletPool に Spawn する処理。TryFire から呼ばれる
    void Fire(const Vector3& origin, const Vector3& direction) override;

public:
    // BulletPool のポインタを受け取り、各種パラメータを設定する
    void Init(BulletPool* pool) override;

    void Uninit() override {}

    const char* GetName() const override { return "Pistol"; }
};
