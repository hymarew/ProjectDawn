#pragma once
#include <string>
#include <unordered_map>

// =====================================================
// SaveData : セーブファイルに永続化するデータの構造体
//
// stages / achievements / unlocks / equipment / settings / stats
// の 6 カテゴリを保持する。
// =====================================================
struct SaveData
{
    // ステージ解放状態。キーは "stage1" / "stage2" / "stage3" など
    std::unordered_map<std::string, bool> stages;

    // 実績解放状態（将来用、現段階は空）
    std::unordered_map<std::string, bool> achievements;

    // 武器の所持状態。キーは "weapon101" など（Inventory と連携）
    std::unordered_map<std::string, bool> unlocks;

    // 装備状態。"primary" / "secondary" → "weapon101" など（WeaponEquip と連携）
    std::unordered_map<std::string, std::string> equipment;

    // オプション設定。"bgmVolume" / "seVolume" / "mouseSensitivity" など（OptionsMenu と連携）
    std::unordered_map<std::string, float> settings;

    // 累計統計。"totalKills" など（AchievementManager と連携）
    std::unordered_map<std::string, int> stats;
};
