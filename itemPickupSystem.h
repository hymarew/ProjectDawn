#pragma once

class Player;
class WorldItemPool;
class ItemDatabase;
class Inventory;
class PendingWeapons;

// =====================================================
// ItemPickupSystem : プレイヤーとワールドアイテムの接触処理
//
//   毎フレーム、アクティブな WorldItem とプレイヤーの距離を判定し、
//   取得半径に入っていたら:
//     - Heal   → その場でプレイヤーを回復（即時使用）
//     - Weapon → PendingWeapons へ仮取得（正式取得はステージクリア時）
//   取得後に ItemPickupEvent を発行し、アイテムをプールへ返却する。
//
// 演出（UI・SE・エフェクト）はイベントのリスナー側が担当するため、
// このクラスは演出の中身を一切知らない。
// =====================================================
class ItemPickupSystem
{
public:
    // inventory は所持済み判定のためだけに参照する（書き込みは Commit 時のみ）
    void Init(const ItemDatabase* itemDB, const Inventory* inventory,
              PendingWeapons* pendingWeapons);

    // GameScene::Update から毎フレーム呼ぶ
    void Update(Player* player, WorldItemPool& pool);

private:
    const ItemDatabase* m_ItemDB         = nullptr;
    const Inventory*    m_Inventory      = nullptr;
    PendingWeapons*     m_PendingWeapons = nullptr;
};
