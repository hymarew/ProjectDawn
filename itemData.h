#pragma once
#include <string>
#include "rarity.h"
#include "weaponData.h"

// =====================================================
// ItemID : アイテムの識別子
//
// 番号帯: 1000番台 = 武器アイテム（1000 + WeaponID 番号）
//         2000番台 = 回復アイテム
//         将来: 3000番台 = 素材、4000番台 = 強化素材 など
// =====================================================
struct ItemID
{
    int number = 0;

    constexpr ItemID() = default;
    constexpr explicit ItemID(int n) : number(n) {}

    constexpr bool operator==(const ItemID& other) const { return number == other.number; }
    constexpr bool operator!=(const ItemID& other) const { return number != other.number; }
    constexpr bool operator<(const ItemID& other)  const { return number <  other.number; }

    bool IsValid() const { return number > 0; }
};

// =====================================================
// ItemType : アイテムの種別
// 将来 Material / EnhanceMaterial / QuestItem などを追加する
// =====================================================
enum class ItemType
{
    Weapon = 0,  // 拾うと Inventory へ武器として追加される
    Heal,        // 拾った瞬間にプレイヤーを回復する（所持しない）
};

inline const char* ItemTypeToString(ItemType type)
{
    switch (type)
    {
    case ItemType::Weapon: return "Weapon";
    case ItemType::Heal:   return "Heal";
    }
    return "Weapon";
}

inline bool ItemTypeFromString(const std::string& str, ItemType& outType)
{
    if (str == "Weapon") { outType = ItemType::Weapon; return true; }
    if (str == "Heal")   { outType = ItemType::Heal;   return true; }
    outType = ItemType::Weapon;
    return false;
}

// =====================================================
// ItemData : アイテム1種分のマスタデータ（JSON 1ファイル = 1アイテム）
//
// 共通フィールド + type 別フィールドの平置き構成。
// 種別が増えて構造体が汚れてきたら type 別パラメータを分離する方針。
// =====================================================
struct ItemData
{
    ItemID      id;
    std::string name;
    ItemType    type   = ItemType::Weapon;
    Rarity      rarity = Rarity::Common;

    std::string modelPath;  // ワールドドロップ時の表示モデル
    std::string iconPath;   // UI アイコン（将来用）

    // ---- type == Weapon のとき有効 ----
    WeaponID weaponID;      // 取得時に Inventory へ追加する武器

    // ---- type == Heal のとき有効 ----
    float healAmount = 0.0f;  // 回復量（HP 実数値）
};
