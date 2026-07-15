#include "pickupEffectPlayer.h"
#include "particleManager.h"
#include "manager.h"
#include "player.h"

void PickupEffectPlayer::Init()
{
    g_ItemPickupEventBus.Subscribe(this);
}

void PickupEffectPlayer::Uninit()
{
    g_ItemPickupEventBus.Unsubscribe(this);
}

void PickupEffectPlayer::OnItemPickedUp(const ItemPickupEvent& ev)
{
    auto& particleManager = ParticleManager::GetInstance();

    switch (ev.type)
    {
    case ItemType::Heal:
    {
        // 回復エフェクトはプレイヤーの身体を中心に出す（旧 HealItem と同じ）
        Player* player = Manager::GetGameObject<Player>();
        Vector3 fxPos = player ? player->GetPosition() : ev.position;
        fxPos.y += 1.0f;
        particleManager.EmitHeal(fxPos);
        break;
    }
    case ItemType::Weapon:
    {
        // 武器取得は拾った場所にきらめきを出す
        Vector3 fxPos = ev.position;
        fxPos.y += 0.5f;
        particleManager.EmitHeal(fxPos);
        break;
    }
    }
}
