#include "worldItemPool.h"
#include "worldItem.h"
#include "debugPrintf.h"

WorldItemPool g_WorldItemPool;

void WorldItemPool::Init(int maxCount)
{
    Uninit();  // 二重 Init 対策

    m_Pool.reserve(maxCount);
    for (int i = 0; i < maxCount; i++)
    {
        WorldItem* item = new WorldItem();
        item->Init();
        m_Pool.push_back(item);
    }
}

void WorldItemPool::Uninit()
{
    for (WorldItem* item : m_Pool)
    {
        item->Uninit();
        delete item;
    }
    m_Pool.clear();
}

void WorldItemPool::Update(float dt)
{
    for (WorldItem* item : m_Pool)
        item->Update(dt);  // 非アクティブは Update 内で即 return する
}

void WorldItemPool::Draw()
{
    for (WorldItem* item : m_Pool)
        item->Draw();
}

void WorldItemPool::DrawShadow()
{
    for (WorldItem* item : m_Pool)
        item->DrawShadow();
}

// ---------------------------------------------------------
// Acquire : 空きスロットを探して返す
// ---------------------------------------------------------
WorldItem* WorldItemPool::Acquire()
{
    for (WorldItem* item : m_Pool)
        if (!item->m_IsActive)
            return item;

    // 満杯。ドロップ1個を諦めるだけでゲーム進行には影響しないため警告のみ
    DebugPrintf("[WorldItemPool] pool exhausted (%d items)\n", (int)m_Pool.size());
    return nullptr;
}

int WorldItemPool::GetActiveCount() const
{
    int count = 0;
    for (const WorldItem* item : m_Pool)
        if (item->m_IsActive) count++;
    return count;
}
