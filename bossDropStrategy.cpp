#include "bossDropStrategy.h"

void BossDropStrategy::Select(const DropTableData& table, std::vector<ItemID>& outItems)
{
    // 確定ドロップ: rollCount 個を必ず抽選する
    for (int i = 0; i < table.rollCount; i++)
    {
        ItemID id = PickWeighted(table.entries);
        if (id.IsValid())
            outItems.push_back(id);
    }
}
