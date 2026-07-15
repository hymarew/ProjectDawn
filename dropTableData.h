#pragma once
#include <string>
#include <vector>
#include "itemData.h"

// =====================================================
// DropTableData : 敵1種分のドロップテーブル（JSON 1ファイル = 1テーブル）
//
// enemyTypeName は Enemy::GetTypeName()（"Scorpion" / "Spider" 等）と対応し、
// DropTableDatabase の検索キーになる。
// 抽選アルゴリズム自体は strategyType で選ばれる IDropStrategy が担当する。
// =====================================================

// 抽選対象1件分
struct DropEntry
{
    ItemID itemID;
    int    weight = 1;  // 重み。大きいほど選ばれやすい
};

struct DropTableData
{
    std::string enemyTypeName;              // "Scorpion" 等。Enemy::GetTypeName() と一致させる
    std::string strategyType = "Weighted";  // 抽選方法（"Weighted" / "Boss"）
    float       dropRate     = 0.0f;        // ドロップ発生率（0〜1）。Boss では未使用（確定ドロップ）
    int         rollCount    = 1;           // 抽選回数。Boss で「確定 N 個ドロップ」に使う
    std::vector<DropEntry> entries;
};
