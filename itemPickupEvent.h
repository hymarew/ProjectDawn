#pragma once
#include <string>
#include <vector>
#include "itemData.h"
#include "vector3.h"

// =====================================================
// ItemPickupEvent : アイテム取得通知（Observer パターン）
//
// ItemPickupSystem が取得処理（回復適用 / Inventory 追加）を終えた後に
// Publish し、UI 通知・エフェクト・SE・実績などのリスナーが受け取る。
//
// 演出を増やすときは IItemPickupListener の実装を1つ作って
// Subscribe するだけでよく、取得処理本体は一切修正しない。
// =====================================================
// 武器取得の結果。仮取得方式のため「その場で正式取得」は存在しない
enum class WeaponPickupResult
{
    Pending,         // 仮取得（ステージクリアで正式取得）
    AlreadyOwned,    // Inventory に所持済み
    AlreadyPending,  // このステージで仮取得済み
};

struct ItemPickupEvent
{
    ItemID      itemID;
    ItemType    type   = ItemType::Heal;
    Rarity      rarity = Rarity::Common;
    std::string name;                    // 表示名（ItemData::name）
    Vector3     position;                // 取得した場所（エフェクト用）

    // type == Weapon のとき有効
    WeaponID           weaponID;
    WeaponPickupResult weaponResult = WeaponPickupResult::Pending;
};

class IItemPickupListener
{
public:
    virtual ~IItemPickupListener() = default;
    virtual void OnItemPickedUp(const ItemPickupEvent& ev) = 0;
};

// =====================================================
// ItemPickupEventBus : リスナー登録と ItemPickupEvent の配信
//
// リスナーの寿命は登録側が管理する（シーン終了時に必ず Unsubscribe すること）。
// =====================================================
class ItemPickupEventBus
{
public:
    void Subscribe(IItemPickupListener* listener);
    void Unsubscribe(IItemPickupListener* listener);
    void Publish(const ItemPickupEvent& ev);

private:
    std::vector<IItemPickupListener*> m_Listeners;
};

extern ItemPickupEventBus g_ItemPickupEventBus;
