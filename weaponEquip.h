#pragma once
#include <memory>
#include "equipLoadout.h"

class Weapon;
class WeaponFactory;
class BulletPool;

// =====================================================
// WeaponEquip : ステージ中の装備武器の実体管理
//
// ステージ開始時に EquipLoadout（装備の永続データ）を読んで
// WeaponFactory で Weapon 実体を生成し、所有する。
// ステージ中にできるのはアクティブスロットの切替（ホイール）のみ。
//
// 装備内容の変更 API は持たない。
// 「ステージ中は装備変更できない」仕様を、メソッドが存在しないことで
// 構造的に保証する（変更は WeaponSelectScene → EquipLoadout でのみ行う）。
// =====================================================
class WeaponEquip
{
public:
    // ステージ開始時に呼ぶ。loadout の内容から Weapon 実体を生成する
    void Init(const EquipLoadout* loadout, const WeaponFactory* factory, BulletPool* pool);
    void Uninit();

    // Primary ⇔ Secondary を切り替える（マウスホイールから呼ぶ）。
    // 切替先が空スロット（1本しか所持していない等）の場合は何もしない
    void SwitchSlot();

    Weapon*   GetActiveWeapon() const { return GetWeapon(m_ActiveSlot); }
    Weapon*   GetWeapon(EquipSlot slot) const;
    EquipSlot GetActiveSlot() const { return m_ActiveSlot; }

private:
    static constexpr int SLOT_COUNT = (int)EquipSlot::Count;

    std::unique_ptr<Weapon> m_Weapons[SLOT_COUNT];
    EquipSlot               m_ActiveSlot = EquipSlot::Primary;
};
