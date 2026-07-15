#include "weightedDropStrategy.h"
#include <cstdlib>

void WeightedDropStrategy::Select(const DropTableData& table, std::vector<ItemID>& outItems)
{
    for (int i = 0; i < table.rollCount; i++)
    {
        // ドロップ発生判定
        if ((float)rand() / RAND_MAX >= table.dropRate) continue;

        ItemID id = PickWeighted(table.entries);
        if (id.IsValid())
            outItems.push_back(id);
    }
}
