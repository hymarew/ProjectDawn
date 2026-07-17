#include "main.h"
#include "creditScreen.h"
#include "input.h"
#include "renderer.h"

// -------------------------------------------------------
// Update : ESC / Enter で閉じる
// -------------------------------------------------------
bool CreditScreen::Update()
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
// Draw : 使用素材の提供元を一覧表示する
// -------------------------------------------------------
void CreditScreen::Draw()
{
    if (!m_IsOpen) return;

    ImDrawList* dl   = ImGui::GetForegroundDrawList();
    ImFont*     font = ImGui::GetFont();

    // ---- 全画面半透明黒オーバーレイ ----
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(0, 0, 0, 200));

    // ---- 表記内容（カテゴリ / 提供元 / URL） ----
    // 魔王魂・効果音ラボの利用規約に基づくクレジット表記。
    // ImGui標準フォントが日本語非対応のためローマ字で表記する
    struct CreditLine { const char* category; const char* name; const char* url; };
    static const CreditLine s_Credits[] = {
        { "BGM",          "MaouDamashii",    "https://maou.audio/" },
        { "Sound Effect", "SoundEffect-Lab", "https://soundeffect-lab.info/" },
    };
    constexpr int kCount = sizeof(s_Credits) / sizeof(s_Credits[0]);

    // ---- パネル ----
    const float rowH    = 92.0f;
    const float PANEL_W = 700.0f;
    const float PANEL_H = 150.0f + rowH * (float)kCount;
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

    // ---- 見出し "CREDIT" ----
    {
        const float sz   = 32.0f;
        const char* text = "CREDIT";
        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, text);
        dl->AddText(font, sz,
            ImVec2(panelX + (PANEL_W - ts.x) * 0.5f, panelY + 24.0f),
            IM_COL32(255, 220, 50, 255), text);
    }

    // ---- 各行（カテゴリ / 提供元 / URL） ----
    const float listY = panelY + 90.0f;
    for (int i = 0; i < kCount; i++)
    {
        const CreditLine& c = s_Credits[i];
        const float y = listY + rowH * i;

        // カテゴリ（左上・小さめ・グレー）
        dl->AddText(font, 18.0f, ImVec2(panelX + 60.0f, y),
            IM_COL32(150, 160, 180, 220), c.category);

        // 提供元名（メイン・白）
        dl->AddText(font, 28.0f, ImVec2(panelX + 60.0f, y + 22.0f),
            IM_COL32(235, 235, 240, 255), c.name);

        // URL（下・青系）
        dl->AddText(font, 18.0f, ImVec2(panelX + 60.0f, y + 58.0f),
            IM_COL32(120, 180, 255, 220), c.url);
    }

    // ---- 閉じる操作のヒント ----
    {
        const char* hint = "ESC / ENTER : Close";
        const float sz   = 16.0f;
        ImVec2 hs = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, hint);
        dl->AddText(font, sz,
            ImVec2(panelX + (PANEL_W - hs.x) * 0.5f, panelY + PANEL_H - 34.0f),
            IM_COL32(140, 140, 150, 200), hint);
    }
}
