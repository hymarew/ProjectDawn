#pragma once
#include "itemData.h"
#include "vector3.h"

class ItemDatabase;
class WorldItem;
class WorldItemPool;

// =====================================================
// ItemFactory : ItemID からワールドアイテムを生成する（Factory Method）
//
//   ItemID → ItemDatabase 検索 → WorldItemPool から取得 → Activate
//
// 呼び出し側（DropManager 等）は生成手順を知らなくてよい。
// =====================================================
class ItemFactory
{
public:
    ItemFactory(const ItemDatabase* database, WorldItemPool* pool);

    // pos の位置にアイテムを出現させる。
    // 失敗（未知の ID / プール満杯）した場合は nullptr を返す。
    WorldItem* Spawn(ItemID id, const Vector3& pos) const;

private:
    const ItemDatabase* m_Database = nullptr;
    WorldItemPool*      m_Pool     = nullptr;
};
