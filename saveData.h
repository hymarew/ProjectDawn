#pragma once
#include <string>
#include <unordered_map>

// =====================================================
// SaveData : セーブファイルに永続化するデータの構造体
//
// 現段階では stages / achievements / unlocks の 3 カテゴリを保持する。
// StageDatabase との連携は Step4-2 で実装する。
// =====================================================
struct SaveData
{
    // ステージ解放状態。キーは "stage1" / "stage2" / "stage3" など
    std::unordered_map<std::string, bool> stages;

    // 実績解放状態（将来用、現段階は空）
    std::unordered_map<std::string, bool> achievements;

    // 武器・アイテム等の解放状態（将来用、現段階は空）
    std::unordered_map<std::string, bool> unlocks;
};
