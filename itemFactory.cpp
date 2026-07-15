#include "itemFactory.h"
#include "itemDatabase.h"
#include "worldItem.h"
#include "worldItemPool.h"
#include "debugPrintf.h"

ItemFactory::ItemFactory(const ItemDatabase* database, WorldItemPool* pool)
    : m_Database(database)
    , m_Pool(pool)
{
}

// ---------------------------------------------------------
// Spawn : ItemID からワールドアイテムを出現させる
// ---------------------------------------------------------
WorldItem* ItemFactory::Spawn(ItemID id, const Vector3& pos) const
{
    if (!m_Database || !m_Pool) return nullptr;

    const ItemData* data = m_Database->Find(id);
    if (!data)
    {
        DebugPrintf("[ItemFactory] unknown ItemID %d\n", id.number);
        return nullptr;
    }

    WorldItem* item = m_Pool->Acquire();
    if (!item) return nullptr;  // プール満杯（Acquire 内で警告済み）

    item->Activate(*data, pos);
    return item;
}
