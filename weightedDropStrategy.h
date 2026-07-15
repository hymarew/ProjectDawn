#pragma once
#include "dropStrategy.h"

// =====================================================
// WeightedDropStrategy : 通常敵用の抽選（"Weighted"）
//
//   1. dropRate（0〜1）の確率でドロップ発生を判定する
//   2. 発生したら entries から重み付きランダムで1件選ぶ
//
// rollCount 回これを繰り返す（通常敵は 1 のまま使う想定）。
// =====================================================
class WeightedDropStrategy : public IDropStrategy
{
public:
    void Select(const DropTableData& table, std::vector<ItemID>& outItems) override;
};
