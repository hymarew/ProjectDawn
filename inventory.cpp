#include "inventory.h"
#include "weaponDatabase.h"
#include "saveManager.h"
#include "debugPrintf.h"
#include "GameConfig.h"

// ---------------------------------------------------------
// Init : セーブから所持状態を復元する
//
// save.json の unlocks セクションに "weapon101": true の形で保存されている。
// 初回起動（所持ゼロ）のときは初期 AR / RL を付与する。
// ---------------------------------------------------------
void Inventory::Init(const WeaponDatabase* weaponDB)
{
    m_WeaponDB = weaponDB;
    m_Weapons.clear();

    // unlocks から "weapon〜" キーを拾って復元する
    for (const auto& kv : g_SaveManager.GetData().unlocks)
    {
        if (!kv.second) continue;  // false = 未所持

        WeaponID id;
        if (!KeyToWeaponID(kv.first, id)) continue;  // 武器以外の unlock キーは無視

        // マスタから消えた武器（JSON 削除等）はセーブに残っていても所持させない
        if (m_WeaponDB && m_WeaponDB->Find(id) == nullptr)
        {
            DebugPrintf("[Inventory] saved weapon %d not in database, skipped\n", id.number);
            continue;
        }

        OwnedWeapon owned;
        owned.weaponID = id;
        owned.isNew    = false;  // 復元した武器は NEW 表示しない
        m_Weapons[id.number] = owned;
    }

    // 初回起動: 初期武器を付与する（AddWeapon 内で保存される）
    if (m_Weapons.empty())
    {
        DebugPrintf("[Inventory] first launch, granting starter weapons\n");
        AddWeapon(WeaponID(GameConfig::WeaponSystem::STARTER_AR_ID));
        AddWeapon(WeaponID(GameConfig::WeaponSystem::STARTER_RL_ID));
    }
}

// ---------------------------------------------------------
// AddWeapon : 武器を追加して即時保存する
// ---------------------------------------------------------
Inventory::AddResult Inventory::AddWeapon(WeaponID id)
{
    if (Has(id)) return AddResult::AlreadyOwned;

    OwnedWeapon owned;
    owned.weaponID = id;
    m_Weapons[id.number] = owned;

    // 取得した瞬間にセーブへ書き込む（StageDatabase::UnlockStage と同じパターン）
    g_SaveManager.GetData().unlocks[WeaponIDToKey(id)] = true;
    g_SaveManager.Save();

    return AddResult::NewlyAcquired;
}

bool Inventory::Has(WeaponID id) const
{
    return m_Weapons.count(id.number) > 0;
}

void Inventory::ClearNewFlag(WeaponID id)
{
    auto it = m_Weapons.find(id.number);
    if (it != m_Weapons.end())
        it->second.isNew = false;
}

std::vector<const Inventory::OwnedWeapon*> Inventory::GetOwnedWeapons() const
{
    std::vector<const OwnedWeapon*> result;
    result.reserve(m_Weapons.size());
    for (const auto& kv : m_Weapons)
        result.push_back(&kv.second);
    return result;  // map 走査なので ID 昇順
}
