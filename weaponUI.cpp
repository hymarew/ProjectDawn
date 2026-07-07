#include "main.h"
#include "weaponUI.h"
#include "player.h"
#include "weapon.h"

void WeaponUI::Draw(const Player* player)
{
    if (!player || !player->IsAlive()) return;

    const Weapon* w = player->GetWeapon();
    if (!w) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    // ---- レイアウト計算（右下固定） ----
    const float panelX = SCREEN_WIDTH  - PANEL_W - MARGIN_R;
    const float panelY = SCREEN_HEIGHT - PANEL_H - MARGIN_B;

    // ---- パネル背景（半透明の黒） ----
    dl->AddRectFilled(
        ImVec2(panelX - 8, panelY - 8),
        ImVec2(panelX + PANEL_W + 8, panelY + PANEL_H + 8),
        IM_COL32(0, 0, 0, 140), 4.0f);

    float curY = panelY;

    // ---- 武器名 ----
    {
        const float nameSize = 18.0f;
        dl->AddText(ImGui::GetFont(), nameSize,
            ImVec2(panelX + 1, curY + 1),
            IM_COL32(0, 0, 0, 180), w->GetName());
        dl->AddText(ImGui::GetFont(), nameSize,
            ImVec2(panelX, curY),
            IM_COL32(220, 220, 220, 255), w->GetName());
        curY += nameSize + 6.0f;
    }

    // ---- 区切り線 ----
    dl->AddLine(
        ImVec2(panelX, curY),
        ImVec2(panelX + PANEL_W, curY),
        IM_COL32(180, 180, 180, 80));
    curY += 8.0f;

    // ---- 残弾数 ----
    {
        const int   mag  = w->GetMagazineSize();
        const int   cur  = w->GetCurrentAmmo();
        const float ammoSize = 28.0f;

        ImU32 ammoCol;
        if (mag == -1)
        {
            ammoCol = IM_COL32(220, 220, 220, 255);  // 無制限: 白
        }
        else if (cur == 0)
        {
            ammoCol = IM_COL32(220, 60, 60, 255);    // 弾切れ: 赤
        }
        else if ((float)cur / (float)mag <= WARN_RATIO)
        {
            ammoCol = IM_COL32(220, 180, 50, 255);   // 警告: 黄色
        }
        else
        {
            ammoCol = IM_COL32(255, 255, 255, 255);  // 通常: 白
        }

        char ammoBuf[32];
        if (mag == -1)
            snprintf(ammoBuf, sizeof(ammoBuf), "∞");
        else
            snprintf(ammoBuf, sizeof(ammoBuf), "%d / %d", cur, mag);

        dl->AddText(ImGui::GetFont(), ammoSize,
            ImVec2(panelX + 1, curY + 1),
            IM_COL32(0, 0, 0, 160), ammoBuf);
        dl->AddText(ImGui::GetFont(), ammoSize,
            ImVec2(panelX, curY),
            ammoCol, ammoBuf);
        curY += ammoSize + 8.0f;
    }

    // ---- リロードゲージ（リロード中のみ） ----
    if (w->IsReloading())
    {
        const float progress = w->GetReloadProgress();

        // "Reloading..." テキスト
        const float labelSize = 14.0f;
        dl->AddText(ImGui::GetFont(), labelSize,
            ImVec2(panelX, curY),
            IM_COL32(160, 210, 255, 220), "Reloading...");
        curY += labelSize + 4.0f;

        // ゲージ外枠
        dl->AddRect(
            ImVec2(panelX, curY),
            ImVec2(panelX + PANEL_W, curY + GAUGE_H),
            IM_COL32(160, 160, 160, 160), 2.0f);

        // ゲージ本体（青緑）
        if (progress > 0.0f)
        {
            dl->AddRectFilled(
                ImVec2(panelX + 1, curY + 1),
                ImVec2(panelX + 1 + (PANEL_W - 2) * progress, curY + GAUGE_H - 1),
                IM_COL32(60, 180, 220, 220), 2.0f);
        }
    }
}
