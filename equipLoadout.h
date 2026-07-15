#pragma once
#include "weaponData.h"

class Inventory;

// =====================================================
// EquipSlot : 装備スロット
// 将来スロットを増やす場合はここに追加する（Count は常に末尾）
// =====================================================
enum class EquipSlot
{
    Primary = 0,
    Secondary,

    Count,
};

inline const char* EquipSlotToString(EquipSlot slot)
{
    switch (slot)
    {
    case EquipSlot::Primary:   return "Primary";
    case EquipSlot::Secondary: return "Secondary";
    default:                   return "Unknown";
    }
}

// =====================================================
// EquipLoadout : 装備の永続データ（スロット → WeaponID）
//
// WeaponSelectScene（ステージ選択前の武器選択画面）が編集し、
// save.json の equipment セクションへ即時保存する。
// ステージ開始時に WeaponEquip がこれを読んで Weapon 実体を生成する。
//
// Weapon 実体・BulletPool・Player を一切知らないため、
// ゲームシーン外（武器選択画面）から安全に編集できる。
// ステージ中の装備変更は「このクラスを GameScene から触らない」ことで防ぐ。
// =====================================================
class EquipLoadout
{
public:
    // セーブから装備を復元し、Inventory と照合して補正する。
    // 未所持・マスタ欠落の装備は外し、空スロットは所持一覧から自動で埋める。
    // GameContext のコンストラクタ（Inventory::Init の後）から呼ばれる。
    void Init(const Inventory* inventory);

    // 所持している武器をスロットへ装備する（未所持なら失敗して false）。
    // 同じ武器が他スロットに装備済みの場合は2スロットの内容を入れ替える。
    // 成功時は save.json へ即時保存する。
    bool Equip(EquipSlot slot, WeaponID id);

    WeaponID GetEquipped(EquipSlot slot) const;

    // 指定武器が装備されているスロットを返す（未装備なら Count）
    EquipSlot FindSlotOf(WeaponID id) const;

    // ゲーム開始可能か（最低 Primary に1本装備されているか）
    bool HasPrimary() const { return m_Slots[(int)EquipSlot::Primary].IsValid(); }

private:
    // 空スロットを所持一覧の先頭から埋める（他スロットとの重複は避ける）
    void FillEmptySlots();

    // 現在の装備状態を save.json へ書き込む
    void Save();

    static constexpr int SLOT_COUNT = (int)EquipSlot::Count;

    WeaponID m_Slots[SLOT_COUNT];

    const Inventory* m_Inventory = nullptr;
};
