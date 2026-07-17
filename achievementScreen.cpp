#include "main.h"
#include "achievementScreen.h"
#include "achievementManager.h"
#include "input.h"
#include "renderer.h"

// -------------------------------------------------------
// Update : ESC / Enter で閉じる
// -------------------------------------------------------
bool AchievementScreen::Update()
{
    if (!m_IsOpen) return false;

    if (Input::GetKeyTrigger(VK_ESCAPE) || Input::GetKeyTrigger(VK_RETURN))
    {
        m_IsOpen = false;
        return true;
    }
    return false;
}

// -------------------------------------------------------
// Draw : 全実績を1列のリストで表示する
// -------------------------------------------------------
void AchievementScreen::Draw()
{
    if (!m_IsOpen) return;

    ImDrawList* dl   = ImGui::GetForegroundDrawList();
    ImFont*     font = ImGui::GetFont();

    // ---- 全画面半透明黒オーバーレイ ----
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(0, 0, 0, 200));

    const auto& defs = AchievementManager::GetDefinitions();

    // ---- パネル ----
    const float rowH    = 58.0f;
    const float PANEL_W = 640.0f;
    const float PANEL_H = 110.0f + rowH * (float)defs.size();
    const float panelX  = (SCREEN_WIDTH  - PANEL_W) * 0.5f;
    const float panelY  = (SCREEN_HEIGHT - PANEL_H) * 0.5f;

    dl->AddRectFilled(
        ImVec2(panelX, panelY),
        ImVec2(panelX + PANEL_W, panelY + PANEL_H),
        IM_COL32(10, 10, 20, 230), 8.0f);
    dl->AddRect(
        ImVec2(panelX, panelY),
        ImVec2(panelX + PANEL_W, panelY + PANEL_H),
        IM_COL32(255, 200, 60, 180), 8.0f, 0, 1.5f);

    // ---- タイトル "ACHIEVEMENTS" + 達成数 ----
    {
        const float sz   = 32.0f;
        const char* text = "ACHIEVEMENTS";
        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, text);
        float tx  = panelX + (PANEL_W - ts.x) * 0.5f;
        float ty  = panelY + 20.0f;

        dl->AddText(font, sz, ImVec2(tx + 2, ty + 2), IM_COL32(0, 0, 0, 160), text);
        dl->AddText(font, sz, ImVec2(tx, ty), IM_COL32(255, 220, 120, 255), text);

        char countBuf[32];
        snprintf(countBuf, sizeof(countBuf), "%d / %d",
            g_AchievementManager.GetUnlockedCount(), (int)defs.size());
        ImVec2 cs = font->CalcTextSizeA(18.0f, FLT_MAX, 0.0f, countBuf);
        dl->AddText(font, 18.0f,
            ImVec2(panelX + (PANEL_W - cs.x) * 0.5f, ty + sz + 4.0f),
            IM_COL32(180, 180, 200, 220), countBuf);
    }

    // ---- 実績リスト ----
    const float listY = panelY + 90.0f;
    for (int i = 0; i < (int)defs.size(); i++)
    {
        const AchievementDef& def      = defs[i];
        const bool            unlocked = g_AchievementManager.IsUnlocked(def.key);
        const float           ry       = listY + rowH * i;

        // 行背景（解放済みはうっすら金色）
        dl->AddRectFilled(
            ImVec2(panelX + 16.0f, ry),
            ImVec2(panelX + PANEL_W - 16.0f, ry + rowH - 8.0f),
            unlocked ? IM_COL32(60, 50, 15, 160) : IM_COL32(25, 25, 35, 160), 5.0f);

        // 解放状態マーク（解放済み: 金の菱形 / 未解放: 灰色の輪郭）
        const float mx = panelX + 40.0f;
        const float my = ry + (rowH - 8.0f) * 0.5f;
        if (unlocked)
            dl->AddNgonFilled(ImVec2(mx, my), 9.0f, IM_COL32(255, 200, 60, 255), 4);
        else
            dl->AddNgon(ImVec2(mx, my), 9.0f, IM_COL32(110, 110, 130, 200), 4, 1.5f);

        // 実績名（未解放は "???" で隠さず名前は見せて挑戦目標にする）
        dl->AddText(font, 21.0f, ImVec2(panelX + 64.0f, ry + 6.0f),
            unlocked ? IM_COL32(255, 230, 150, 255) : IM_COL32(150, 150, 170, 220),
            def.name);

        // 条件説明
        dl->AddText(font, 15.0f, ImVec2(panelX + 64.0f, ry + 30.0f),
            unlocked ? IM_COL32(210, 200, 170, 220) : IM_COL32(120, 120, 140, 200),
            def.desc);
    }

    // ---- 操作ヒント ----
    {
        const char* hint = "ESC : Back";
        ImVec2 hs = font->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, hint);
        dl->AddText(font, 15.0f,
            ImVec2(panelX + (PANEL_W - hs.x) * 0.5f, panelY + PANEL_H - 26.0f),
            IM_COL32(140, 140, 160, 200), hint);
    }
}
