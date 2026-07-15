#include "itemPickupEvent.h"
#include <algorithm>

ItemPickupEventBus g_ItemPickupEventBus;

void ItemPickupEventBus::Subscribe(IItemPickupListener* listener)
{
    if (!listener) return;

    // 二重登録防止（同じリスナーに2回通知しない）
    auto it = std::find(m_Listeners.begin(), m_Listeners.end(), listener);
    if (it == m_Listeners.end())
        m_Listeners.push_back(listener);
}

void ItemPickupEventBus::Unsubscribe(IItemPickupListener* listener)
{
    m_Listeners.erase(
        std::remove(m_Listeners.begin(), m_Listeners.end(), listener),
        m_Listeners.end());
}

void ItemPickupEventBus::Publish(const ItemPickupEvent& ev)
{
    for (IItemPickupListener* listener : m_Listeners)
        listener->OnItemPickedUp(ev);
}
