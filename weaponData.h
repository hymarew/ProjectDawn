#pragma once
#include <string>
#include "rarity.h"

// =====================================================
// WeaponID : 武器の識別子
//
// StageID と同じく番号を保持する軽量な値型。
// JSON の "id" と、セーブキー "weapon101" の数字部分に対応する。
// 番号帯: 101〜 = Assault Rifle / 201〜 = Rocket Launcher（増えても自由）
// =====================================================
struct WeaponID
{
    int number = 0;

    constexpr WeaponID() = default;
    constexpr explicit WeaponID(int n) : number(n) {}

    constexpr bool operator==(const WeaponID& other) const { return number == other.number; }
    constexpr bool operator!=(const WeaponID& other) const { return number != other.number; }
    constexpr bool operator<(const WeaponID& other)  const { return number <  other.number; }

    bool IsValid() const { return number > 0; }
};

// WeaponID(101) → "weapon101"（SaveManager のキーと対応する）
inline std::string WeaponIDToKey(WeaponID id)
{
    return "weapon" + std::to_string(id.number);
}

// "weapon101" → WeaponID(101)。形式不正なら false。
inline bool KeyToWeaponID(const std::string& key, WeaponID& outID)
{
    const std::string prefix = "weapon";
    if (key.compare(0, prefix.size(), prefix) != 0) return false;

    std::string numPart = key.substr(prefix.size());
    if (numPart.empty()) return false;
    for (char c : numPart)
        if (c < '0' || c > '9') return false;

    outID = WeaponID(std::stoi(numPart));
    return true;
}

enum class WeaponCategory
{
    AssaultRifle = 0,
    RocketLauncher,
};

inline const char* WeaponCategoryToString(WeaponCategory category)
{
    switch (category)
    {
    case WeaponCategory::AssaultRifle:   return "AssaultRifle";
    case WeaponCategory::RocketLauncher: return "RocketLauncher";
    }
    return "AssaultRifle";
}

inline bool WeaponCategoryFromString(const std::string& str, WeaponCategory& outCategory)
{
    if (str == "AssaultRifle")   { outCategory = WeaponCategory::AssaultRifle;   return true; }
    if (str == "RocketLauncher") { outCategory = WeaponCategory::RocketLauncher; return true; }
    outCategory = WeaponCategory::AssaultRifle;
    return false;
}

// =====================================================
// WeaponData : 武器1種分のマスタデータ（JSON 1ファイル = 1武器）
//
// 旧 WeaponParams（武器挙動）+ BulletParams（弾性能）を統合したもの。
// ロジックは持たない純粋なパラメータの入れ物。
// Weapon 実体は WeaponFactory がこのデータと AttackStrategy から組み立てる。
// =====================================================
struct WeaponData
{
    WeaponID       id;
    std::string    name;                       // UI 表示名（英字。ImGui 標準フォントは日本語非対応）
    WeaponCategory category = WeaponCategory::AssaultRifle;
    Rarity         rarity   = Rarity::Common;

    // ---- 武器挙動（旧 WeaponParams） ----
    int   magazineSize = 30;     // 弾倉の最大装弾数。-1 で弾数無制限
    float fireRate     = 0.1f;   // 発射間隔（秒）
    float reloadTime   = 2.0f;   // リロード時間（秒）
    float zoomRatio    = 1.0f;   // ADS 時の画角倍率（1.0 でズームなし）
    bool  singleShot   = false;  // true で押した瞬間のみ発射

    // ---- 弾性能（旧 BulletParams） ----
    float damage         = 10.0f;  // 1発あたりのダメージ
    float bulletSpeed    = 50.0f;  // 弾速（単位/秒）
    float bulletLifeTime = 2.0f;   // 弾寿命（秒）= 実質射程
    float spreadAngle    = 0.0f;   // 拡散角（度）
    float splashRadius   = 0.0f;   // 爆風半径（0 = 直撃のみ）
    float knockbackPower = 0.0f;   // 爆発ノックバック力

    // ---- 生成・表示 ----
    std::string attackType = "StraightShot";  // WeaponFactory が Strategy を選ぶキー
    std::string modelPath;                    // 装備モデル（将来用。現状未使用）
    std::string iconPath;                     // UI アイコン（将来用。現状未使用）
};
