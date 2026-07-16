#include "equipLoadout.h"
#include "inventory.h"
#include "saveManager.h"
#include "debugPrintf.h"

namespace
{
    // save.json の equipment セクションで使うキー
    const char* SlotToSaveKey(EquipSlot slot)
    {
        return (slot == EquipSlot::Primary) ? "primary" : "secondary";
    }
}

// ---------------------------------------------------------
// Init : セーブから装備を復元して Inventory と照合する
// ---------------------------------------------------------
void EquipLoadout::Init(const Inventory* inventory)
{
    m_Inventory = inventory;

    // ---- セーブから装備 WeaponID を復元する ----
    const auto& equipment = g_SaveManager.GetData().equipment;
    for (int i = 0; i < SLOT_COUNT; i++)
    {
        m_Slots[i] = WeaponID();  // 無効値で初期化

        auto it = equipment.find(SlotToSaveKey((EquipSlot)i));
        if (it == equipment.end()) continue;

        WeaponID id;
        if (KeyToWeaponID(it->second, id) && m_Inventory && m_Inventory->Has(id))
            m_Slots[i] = id;
        else
            DebugPrintf("[EquipLoadout] saved equipment '%s' is invalid, cleared\n",
                        it->second.c_str());
    }

    FillEmptySlots();
    Save();
}

// ---------------------------------------------------------
// Equip : スロットへ装備する（他スロットと重複したら入れ替え）
// ---------------------------------------------------------
bool EquipLoadout::Equip(EquipSlot slot, WeaponID id)
{
    if (slot >= EquipSlot::Count) return false;
    if (!m_Inventory || !m_Inventory->Has(id))
    {
        DebugPrintf("[EquipLoadout] tried to equip unowned weapon %d\n", id.number);
        return false;
    }

    // 同じ武器を同じスロットへ → 何もしない
    if (m_Slots[(int)slot] == id) return true;

    // 同じ武器が他スロットに装備済み → 2スロットの内容を入れ替える
    EquipSlot otherSlot = FindSlotOf(id);
    if (otherSlot != EquipSlot::Count)
        m_Slots[(int)otherSlot] = m_Slots[(int)slot];

    m_Slots[(int)slot] = id;

    Save();
    return true;
}

WeaponID EquipLoadout::GetEquipped(EquipSlot slot) const
{
    if (slot >= EquipSlot::Count) return WeaponID();
    return m_Slots[(int)slot];
}

EquipSlot EquipLoadout::FindSlotOf(WeaponID id) const
{
    if (!id.IsValid()) return EquipSlot::Count;
    for (int i = 0; i < SLOT_COUNT; i++)
        if (m_Slots[i] == id)
            return (EquipSlot)i;
    return EquipSlot::Count;
}

// ---------------------------------------------------------
// FillEmptySlots : 空スロットを所持一覧の先頭から埋める
// （初回起動、またはセーブ上の装備武器が無効になったときの保険）
// ---------------------------------------------------------
void EquipLoadout::FillEmptySlots()
{
    if (!m_Inventory) return;

    auto owned = m_Inventory->GetOwnedWeapons();
    size_t nextIndex = 0;
    for (int i = 0; i < SLOT_COUNT; i++)
    {
        if (m_Slots[i].IsValid()) continue;

        // 他スロットに装備済みの武器はスキップして次の所持武器を探す
        while (nextIndex < owned.size())
        {
            WeaponID candidate = owned[nextIndex]->weaponID;
            nextIndex++;
            if (FindSlotOf(candidate) == EquipSlot::Count)
            {
                m_Slots[i] = candidate;
                break;
            }
        }
    }
}

// ---------------------------------------------------------
// Save : 装備状態を save.json へ即時保存する
// ---------------------------------------------------------
void EquipLoadout::Save()
{
    auto& equipment = g_SaveManager.GetData().equipment;
    for (int i = 0; i < SLOT_COUNT; i++)
    {
        if (m_Slots[i].IsValid())
            equipment[SlotToSaveKey((EquipSlot)i)] = WeaponIDToKey(m_Slots[i]);
        else
            equipment.erase(SlotToSaveKey((EquipSlot)i));
    }
    g_SaveManager.Save();
}
