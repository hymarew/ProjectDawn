#include "pendingWeapons.h"
#include "inventory.h"
#include "debugPrintf.h"
#include <algorithm>

// ---------------------------------------------------------
// Add : 仮取得を試みる（所持済み・仮取得済みは弾く）
// ---------------------------------------------------------
PendingWeapons::AddResult PendingWeapons::Add(WeaponID id, const Inventory& inventory)
{
    if (inventory.Has(id)) return AddResult::AlreadyOwned;
    if (Contains(id))      return AddResult::AlreadyPending;

    m_Pending.push_back(id);
    return AddResult::Pending;
}

// ---------------------------------------------------------
// CommitTo : クリア時の正式取得。全件を Inventory へ移して空にする
// ---------------------------------------------------------
void PendingWeapons::CommitTo(Inventory& inventory)
{
    for (const WeaponID& id : m_Pending)
        inventory.AddWeapon(id);  // 1件ごとに save.json へ即時保存される

    DebugPrintf("[PendingWeapons] committed %d weapons\n", (int)m_Pending.size());
    m_Pending.clear();
}

bool PendingWeapons::Contains(WeaponID id) const
{
    return std::find(m_Pending.begin(), m_Pending.end(), id) != m_Pending.end();
}
