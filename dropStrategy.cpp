#include "dropStrategy.h"
#include <cstdlib>

// ---------------------------------------------------------
// PickWeighted : 重み付きランダムで1件選ぶ
//
// 全エントリの重み合計から乱数を引き、順に減算して当たりを決める。
// 例: weight が [50, 30, 20] なら 50% / 30% / 20% で選ばれる。
// ---------------------------------------------------------
ItemID IDropStrategy::PickWeighted(const std::vector<DropEntry>& entries)
{
    if (entries.empty()) return ItemID();

    int totalWeight = 0;
    for (const DropEntry& entry : entries)
        totalWeight += entry.weight;
    if (totalWeight <= 0) return ItemID();

    int roll = rand() % totalWeight;
    for (const DropEntry& entry : entries)
    {
        roll -= entry.weight;
        if (roll < 0) return entry.itemID;
    }

    // 丸め誤差はないはずだが、念のため最後のエントリを返す
    return entries.back().itemID;
}
