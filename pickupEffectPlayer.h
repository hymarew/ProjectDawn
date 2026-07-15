#pragma once
#include "itemPickupEvent.h"

// =====================================================
// PickupEffectPlayer : アイテム取得エフェクト再生（Observer のリスナー）
//
// 取得イベントを受け取り、種別に応じたパーティクルを再生する。
//   Heal   → 緑の回復エフェクト（旧 HealItem と同じ演出）
//   Weapon → 取得位置に回復と同系のきらめき
//
// SE 再生を追加する場合もこのクラス（または新リスナー）に足すだけでよく、
// 取得処理本体（ItemPickupSystem）は修正不要。
// =====================================================
class PickupEffectPlayer : public IItemPickupListener
{
public:
    void Init();    // EventBus へ Subscribe する
    void Uninit();  // EventBus から Unsubscribe する

    // IItemPickupListener
    void OnItemPickedUp(const ItemPickupEvent& ev) override;
};
