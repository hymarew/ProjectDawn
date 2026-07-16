#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include "weaponData.h"

class Weapon;
class WeaponDatabase;
class BulletPool;
class IAttackStrategy;

// =====================================================
// WeaponFactory : WeaponID から Weapon 実体を生成する（Factory Method）
//
//   WeaponID → WeaponDatabase 検索 → attackType 文字列で Strategy を選択
//            → Weapon(データ × Strategy) を組み立てて返す
//
// attackType → Strategy の対応はコンストラクタの登録表で管理する。
// 新しい攻撃方式を追加するときは登録表に1行足すだけでよい。
// =====================================================
class WeaponFactory
{
public:
    explicit WeaponFactory(const WeaponDatabase* database);

    // 生成に失敗（未知の ID / 未知の attackType）した場合は nullptr を返す
    std::unique_ptr<Weapon> Create(WeaponID id, BulletPool* pool) const;

    // Database ロード後のバリデーション用: attackType が登録表にあるか
    bool IsKnownAttackType(const std::string& attackType) const;

    // 全武器の attackType を検証し、未知のものはログへ警告する。
    // 戻り値: 未知の attackType が1件もなければ true
    bool ValidateDatabase() const;

private:
    const WeaponDatabase* m_Database = nullptr;

    // attackType 文字列 → Strategy 生成関数 の登録表
    using StrategyCreator = std::function<std::unique_ptr<IAttackStrategy>()>;
    std::unordered_map<std::string, StrategyCreator> m_StrategyRegistry;
};
