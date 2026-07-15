#pragma once
#include "dropStrategy.h"

// =====================================================
// BossDropStrategy : ボス用の抽選（"Boss"）
//
// dropRate を使わず、rollCount 回の重み付き抽選を確定で行う。
// 「ボスを倒したのに何も出ない」を防ぎ、必ず rollCount 個ドロップする。
// =====================================================
class BossDropStrategy : public IDropStrategy
{
public:
    void Select(const DropTableData& table, std::vector<ItemID>& outItems) override;
};
