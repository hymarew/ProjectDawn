#pragma once
#include "vector3.h"

struct WeaponData;
class BulletPool;

// =====================================================
// IAttackStrategy : 武器の攻撃処理インターフェース（Strategy パターン）
//
// 「1発撃つと何が起きるか（弾の Spawn の仕方）」だけを担当する。
// 連射間隔・残弾・リロードの管理は Weapon 側の仕事なので持たない。
//
// 新しい攻撃方式（拡散ショット・チャージ弾など）を追加するときは
//   1. このインターフェースの実装クラスを1つ作る
//   2. WeaponFactory の登録表に attackType 文字列と対応付けて1行追加する
// だけでよく、既存の武器・Weapon クラスは修正不要。
// =====================================================
class IAttackStrategy
{
public:
    virtual ~IAttackStrategy() = default;

    // TryFire の条件チェック通過後に Weapon から呼ばれる
    virtual void Fire(const Vector3& origin, const Vector3& direction,
                      const WeaponData& data, BulletPool& pool) = 0;
};
