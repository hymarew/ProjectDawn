#pragma once
#include "vector3.h"

// =====================================================
// EnemyProjectileType : 敵弾の種別
//
// 弾種ごとにモデル・パラメータが異なる。
// 現在は Needle のみ実装。
// =====================================================
enum class EnemyProjectileType
{
    Needle,   // サソリ毒針 : bullet.obj
    Acid,     // 飛行敵酸   : 未実装
    Fireball, // 大型敵火球 : 未実装
    Rocket,   // ボスロケット: 未実装
    Laser,    // ボスレーザー: 未実装
};

// =====================================================
// EnemyProjectile : 敵弾1発分のデータ
// =====================================================
struct EnemyProjectile
{
    Vector3              position  = {};
    Vector3              velocity  = {};
    float                damage    = 0.0f;
    float                lifeTime  = 0.0f;
    float                radius    = 0.0f;
    EnemyProjectileType  type      = EnemyProjectileType::Needle;
    bool                 isActive  = false;
};
