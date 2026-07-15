#include "weaponEquip.h"
#include "weapon.h"
#include "weaponFactory.h"
#include "debugPrintf.h"

// ---------------------------------------------------------
// Init : EquipLoadout の装備内容から Weapon 実体を生成する
// ---------------------------------------------------------
void WeaponEquip::Init(const EquipLoadout* loadout, const WeaponFactory* factory,
                       BulletPool* pool)
{
    m_ActiveSlot = EquipSlot::Primary;

    for (int i = 0; i < SLOT_COUNT; i++)
    {
        m_Weapons[i].reset();

        if (!loadout || !factory) continue;

        WeaponID id = loadout->GetEquipped((EquipSlot)i);
        if (!id.IsValid()) continue;  // 空スロット（Secondary 未装備等）は許容する

        m_Weapons[i] = factory->Create(id, pool);
        if (!m_Weapons[i])
            DebugPrintf("[WeaponEquip] failed to create weapon %d for slot %d\n",
                        id.number, i);
    }
}

void WeaponEquip::Uninit()
{
    for (int i = 0; i < SLOT_COUNT; i++)
        m_Weapons[i].reset();
    m_ActiveSlot = EquipSlot::Primary;
}

// ---------------------------------------------------------
// SwitchSlot : Primary ⇔ Secondary の切替（空スロットへは切り替えない）
// ---------------------------------------------------------
void WeaponEquip::SwitchSlot()
{
    EquipSlot next = (m_ActiveSlot == EquipSlot::Primary)
                   ? EquipSlot::Secondary : EquipSlot::Primary;

    if (GetWeapon(next) == nullptr) return;  // 切替先に武器がなければ維持する

    m_ActiveSlot = next;
}

Weapon* WeaponEquip::GetWeapon(EquipSlot slot) const
{
    if (slot >= EquipSlot::Count) return nullptr;
    return m_Weapons[(int)slot].get();
}
