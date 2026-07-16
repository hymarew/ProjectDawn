#include "itemPickupSystem.h"
#include "worldItem.h"
#include "worldItemPool.h"
#include "itemDatabase.h"
#include "inventory.h"
#include "pendingWeapons.h"
#include "itemPickupEvent.h"
#include "player.h"
#include "GameConfig.h"

void ItemPickupSystem::Init(const ItemDatabase* itemDB, const Inventory* inventory,
                            PendingWeapons* pendingWeapons)
{
    m_ItemDB         = itemDB;
    m_Inventory      = inventory;
    m_PendingWeapons = pendingWeapons;
}

namespace
{
    // PendingWeapons の追加結果をイベント用の結果へ変換する
    WeaponPickupResult ToPickupResult(PendingWeapons::AddResult result)
    {
        switch (result)
        {
        case PendingWeapons::AddResult::AlreadyOwned:   return WeaponPickupResult::AlreadyOwned;
        case PendingWeapons::AddResult::AlreadyPending: return WeaponPickupResult::AlreadyPending;
        default:                                        return WeaponPickupResult::Pending;
        }
    }
}

// ---------------------------------------------------------
// Update : 接触判定 → 取得処理 → イベント発行
// ---------------------------------------------------------
void ItemPickupSystem::Update(Player* player, WorldItemPool& pool)
{
    if (!player || !m_ItemDB || !m_Inventory || !m_PendingWeapons) return;

    const float pickupDist = GameConfig::WorldItem::PICKUP_RADIUS
                           + GameConfig::Collision::PLAYER_RADIUS;
    const float pickupDistSq = pickupDist * pickupDist;
    const Vector3 playerPos = player->GetPosition();

    for (WorldItem* item : pool.GetItems())
    {
        if (!item->m_IsActive) continue;

        // 浮遊で Y がずれるため水平距離で判定する
        Vector3 diff = playerPos - item->GetPosition();
        diff.y = 0.0f;
        if (diff.LengthSq() > pickupDistSq) continue;

        const ItemData* data = m_ItemDB->Find(item->GetItemID());
        if (!data)
        {
            // マスタにないアイテム（参照切れ）。拾えないままにせず消しておく
            item->Deactivate();
            continue;
        }

        // ---- 取得処理本体（種別ごとの効果適用） ----
        ItemPickupEvent ev;
        ev.itemID   = data->id;
        ev.type     = data->type;
        ev.rarity   = data->rarity;
        ev.name     = data->name;
        ev.position = item->GetPosition();

        switch (data->type)
        {
        case ItemType::Heal:
            // 回復は即時使用（Inventory を通さない）
            player->Heal(data->healAmount);
            break;

        case ItemType::Weapon:
            // 武器は仮取得。正式取得はステージクリア時の CommitTo で行われ、
            // ゲームオーバー・途中終了では失われる
            ev.weaponID     = data->weaponID;
            ev.weaponResult = ToPickupResult(
                m_PendingWeapons->Add(data->weaponID, *m_Inventory));
            break;
        }

        // ---- 演出はリスナー（UI・エフェクト等）へ委譲 ----
        g_ItemPickupEventBus.Publish(ev);

        item->Deactivate();  // プールへ返却
    }
}
