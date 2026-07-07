#include "main.h"
#include "pauseMenu.h"
#include "input.h"
#include "renderer.h"

// -------------------------------------------------------
// Open : ポーズ開始時に選択状態をリセットする
// -------------------------------------------------------
void PauseMenu::Open()
{
    m_SelectedIndex = 0;
    m_ResumeReq     = false;
    m_RetryReq      = false;
    m_TitleReq      = false;
    m_ExitReq       = false;
}

// -------------------------------------------------------
// Close : ポーズ解除時に呼ぶ（現状は特に処理なし）
// -------------------------------------------------------
void PauseMenu::Close()
{
    m_ResumeReq = false;
    m_RetryReq  = false;
    m_TitleReq  = false;
    m_ExitReq   = false;
}

// -------------------------------------------------------
// Update : メニュー上下移動・決定の入力処理
// -------------------------------------------------------
void PauseMenu::Update()
{
    // リクエストフラグを毎フレームリセット
    m_ResumeReq = false;
    m_RetryReq  = false;
    m_TitleReq  = false;
    m_ExitReq   = false;

    // カーソル移動（↑ W / ↓ S）
    if (Input::GetKeyTrigger(VK_UP)   || Input::GetKeyTrigger('W'))
    {
        m_SelectedIndex = (m_SelectedIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
    }
    if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
    {
        m_SelectedIndex = (m_SelectedIndex + 1) % ITEM_COUNT;
    }

    // 決定（Enter / Space）
    if (Input::GetKeyTrigger(VK_RETURN) || Input::GetKeyTrigger(VK_SPACE))
    {
        switch (m_SelectedIndex)
        {
        case 0: m_ResumeReq = true; break;
        case 1: m_RetryReq  = true; break;
        case 2: m_TitleReq  = true; break;
        case 3: m_ExitReq   = true; break;
        }
    }
}

// -------------------------------------------------------
// Draw : 半透明オーバーレイ + メニューパネル描画
// -------------------------------------------------------
void PauseMenu::Draw()
{
    // ForegroundDrawList を使うことでクロスヘア・ダメージ表示より前面に描画される
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // ---- 全画面半透明黒オーバーレイ ----
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(0, 0, 0, 128));

    // ---- パネル ----
    constexpr float PANEL_W = 300.0f;
    constexpr float PANEL_H = 330.0f; // Exit 追加で1項目分（54px）増やした
    const float panelX = (SCREEN_WIDTH  - PANEL_W) * 0.5f;
    const float panelY = (SCREEN_HEIGHT - PANEL_H) * 0.5f;

    // パネル背景
    dl->AddRectFilled(
        ImVec2(panelX, panelY),
        ImVec2(panelX + PANEL_W, panelY + PANEL_H),
        IM_COL32(10, 10, 20, 210), 8.0f);

    // パネル枠
    dl->AddRect(
        ImVec2(panelX, panelY),
        ImVec2(panelX + PANEL_W, panelY + PANEL_H),
        IM_COL32(100, 140, 220, 200), 8.0f, 0, 1.5f);

    // ---- タイトル "PAUSE" ----
    {
        const float sz   = 36.0f;
        const char* text = "PAUSE";
        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(sz, FLT_MAX, 0.0f, text);
        float tx  = panelX + (PANEL_W - ts.x) * 0.5f;
        float ty  = panelY + 28.0f;

        dl->AddText(ImGui::GetFont(), sz, ImVec2(tx + 2, ty + 2),
            IM_COL32(0, 0, 0, 160), text);
        dl->AddText(ImGui::GetFont(), sz, ImVec2(tx, ty),
            IM_COL32(180, 200, 255, 255), text);
    }

    // 区切り線
    dl->AddLine(
        ImVec2(panelX + 20.0f, panelY + 78.0f),
        ImVec2(panelX + PANEL_W - 20.0f, panelY + 78.0f),
        IM_COL32(80, 100, 180, 160), 1.0f);

    // ---- メニュー項目 ----
    const char* labels[] = { "Resume", "Retry", "Title", "Exit" };
    const float itemSz    = 28.0f;
    const float itemStart = panelY + 100.0f;
    const float itemStep  = 54.0f;

    for (int i = 0; i < ITEM_COUNT; i++)
    {
        float iy = itemStart + itemStep * i;

        const bool selected = (i == m_SelectedIndex);

        // 選択ハイライト背景
        if (selected)
        {
            dl->AddRectFilled(
                ImVec2(panelX + 14.0f, iy - 6.0f),
                ImVec2(panelX + PANEL_W - 14.0f, iy + itemSz + 2.0f),
                IM_COL32(60, 100, 200, 120), 4.0f);
        }

        // カーソル "▶"
        if (selected)
        {
            dl->AddText(ImGui::GetFont(), itemSz,
                ImVec2(panelX + 24.0f, iy),
                IM_COL32(120, 180, 255, 255), ">");
        }

        // ラベル
        ImVec2 ls = ImGui::GetFont()->CalcTextSizeA(itemSz, FLT_MAX, 0.0f, labels[i]);
        float  lx = panelX + (PANEL_W - ls.x) * 0.5f;

        ImU32 col = selected
            ? IM_COL32(255, 255, 255, 255)
            : IM_COL32(160, 160, 180, 200);

        dl->AddText(ImGui::GetFont(), itemSz, ImVec2(lx, iy), col, labels[i]);
    }
}
