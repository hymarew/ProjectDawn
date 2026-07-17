#pragma once
#include "vector3.h"

// =====================================================
// Bullet : BulletPool が管理する弾1発分のデータ
// =====================================================
struct Bullet
{
    Vector3 position     = {};
    Vector3 prevPosition = {}; // 前フレームの位置。弾道の線分判定（すり抜け防止）に使う
    Vector3 velocity     = {};
    float   lifeTime    = 0.0f;
    float   damage      = 0.0f;
    float   splashRadius   = 0.0f; // 0 = 直撃ダメージのみ、>0 = 着弾時に半径内全体にダメージ
    float   knockbackPower = 0.0f; // 爆発時に敵を外側へ吹き飛ばす力（0 = ノックバックなし）
    bool    isActive       = false;

    // ---- ロケット弾専用の演出用ステート（splashRadius > 0 のときのみ使用） ----
    int   lightSlot    = -1;   // DynamicLightManager に確保した追従ライトのスロット（-1 = 未確保）
    float sparkTimer   = 0.0f; // 次の火花生成までのカウントダウン
};
