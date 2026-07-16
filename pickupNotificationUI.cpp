#include "main.h"
#include "pickupNotificationUI.h"
#include "imgui/imgui.h"

void PickupNotificationUI::Init()
{
    m_Toasts.clear();
    g_ItemPickupEventBus.Subscribe(this);
}

void PickupNotificationUI::Uninit()
{
    g_ItemPickupEventBus.Unsubscribe(this);
    m_Toasts.clear();
}

// ---------------------------------------------------------
// OnItemPickedUp : 取得イベントをトーストに変換してキューへ積む
// ---------------------------------------------------------
void PickupNotificationUI::OnItemPickedUp(const ItemPickupEvent& ev)
{
    Toast toast;
    toast.body   = ev.name;
    toast.rarity = ev.rarity;
    toast.timer  = DISPLAY_TIME;

    switch (ev.type)
    {
    case ItemType::Weapon:
        // 仮取得は「クリアで正式取得」であることをトーストで伝える
        switch (ev.weaponResult)
        {
        case WeaponPickupResult::Pending:        toast.header = "NEW WEAPON! (ON CLEAR)"; break;
        case WeaponPickupResult::AlreadyOwned:   toast.header = "ALREADY OWNED";          break;
        case WeaponPickupResult::AlreadyPending: toast.header = "ALREADY FOUND";          break;
        }
        break;
    case ItemType::Heal:
        toast.header = "RECOVERED";
        break;
    }

    m_Toasts.push_back(toast);

    // 上限を超えたら古いものから捨てる
    while ((int)m_Toasts.size() > MAX_TOASTS)
        m_Toasts.erase(m_Toasts.begin());
}

void PickupNotificationUI::Update(float dt)
{
    for (Toast& toast : m_Toasts)
        toast.timer -= dt;

    m_Toasts.erase(
        std::remove_if(m_Toasts.begin(), m_Toasts.end(),
            [](const Toast& t) { return t.timer <= 0.0f; }),
        m_Toasts.end());
}

// ---------------------------------------------------------
// Draw : 画面左中段にトーストを縦に並べて描画する
// ---------------------------------------------------------
void PickupNotificationUI::Draw()
{
    if (m_Toasts.empty()) return;

    ImDrawList* dl   = ImGui::GetBackgroundDrawList();
    ImFont*     font = ImGui::GetFont();

    const float x      = 20.0f;
    const float startY = SCREEN_HEIGHT * 0.35f;
    const float boxW   = 260.0f;
    const float boxH   = 46.0f;
    const float gap    = 8.0f;

    float y = startY;
    for (const Toast& toast : m_Toasts)
    {
        // 残り時間が FADE_TIME を切ったらフェードアウトする
        float alpha = (toast.timer < FADE_TIME) ? (toast.timer / FADE_TIME) : 1.0f;
        int a = (int)(alpha * 255.0f);

        RarityColor color = GetRarityColor(toast.rarity);
        ImU32 rarityCol = IM_COL32((int)(color.r * 255), (int)(color.g * 255),
                                   (int)(color.b * 255), a);

        // 背景パネル + レアリティ色の左縁ライン
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + boxW, y + boxH),
            IM_COL32(20, 20, 25, (int)(alpha * 190.0f)), 4.0f);
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + 4.0f, y + boxH), rarityCol, 4.0f);
        dl->AddRect(ImVec2(x, y), ImVec2(x + boxW, y + boxH),
            IM_COL32(120, 120, 130, (int)(alpha * 150.0f)), 4.0f);

        // ヘッダー（小・グレー）+ アイテム名（大・レアリティ色）
        dl->AddText(font, 13.0f, ImVec2(x + 12.0f, y + 5.0f),
            IM_COL32(190, 190, 190, a), toast.header.c_str());
        dl->AddText(font, 19.0f, ImVec2(x + 12.0f, y + 21.0f),
            rarityCol, toast.body.c_str());

        y += boxH + gap;
    }
}
