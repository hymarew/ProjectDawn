#pragma once
#include <string>
#include <vector>
#include "itemPickupEvent.h"

// =====================================================
// PickupNotificationUI : アイテム取得トースト表示（Observer のリスナー）
//
// EventBus から取得通知を受け取ってキューに積み、
// 画面左中段に「NEW WEAPON! AR-2 Falcon」のような
// トーストを一定時間表示する。
//
// GameScene が所有し、Init/Uninit で Subscribe/Unsubscribe する。
// =====================================================
class PickupNotificationUI : public IItemPickupListener
{
public:
    void Init();    // EventBus へ Subscribe する
    void Uninit();  // EventBus から Unsubscribe し、キューを空にする

    void Update(float dt);
    void Draw();    // ImGui::Render() 前に呼ぶ（BackgroundDrawList 使用）

    // IItemPickupListener
    void OnItemPickedUp(const ItemPickupEvent& ev) override;

private:
    struct Toast
    {
        std::string header;   // "NEW WEAPON!" / "RECOVERED" / "ALREADY OWNED"
        std::string body;     // アイテム名
        Rarity      rarity = Rarity::Common;
        float       timer  = 0.0f;  // 残り表示時間（秒）
    };

    std::vector<Toast> m_Toasts;

    static constexpr float DISPLAY_TIME = 3.0f;   // 1件あたりの表示時間（秒）
    static constexpr float FADE_TIME    = 0.5f;   // 消える前のフェード時間（秒）
    static constexpr int   MAX_TOASTS   = 5;      // 同時表示の上限（超えたら古い順に消す）
};
