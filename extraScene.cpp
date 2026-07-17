// ===================================================
// extraScene.cpp
// 高難易度コンテンツ選択画面の実装
// ===================================================

#include "main.h"
#include "extraScene.h"
#include "sceneManager.h"
#include "gameContext.h"
#include "transitionManager.h"
#include "renderer.h"
#include "input.h"

void ExtraScene::Init()
{
    m_Selected = 0;
    BuildItems();
    ShowCursor(TRUE);
}

void ExtraScene::Uninit()
{
}

// ---------------------------------------------------------
// BuildItems : EXTRAモードの一覧を構築する
// 新モードの追加はここに1エントリ足すだけでよい
// ---------------------------------------------------------
void ExtraScene::BuildItems()
{
    m_Items.clear();

    // ENDLESS: 敵のWaveが無限に続く生存モード（実装済み）
    m_Items.push_back({
        "ENDLESS", "Survive endless waves of enemies",
        nullptr, nullptr,
        []() {
            GameContext::Instance().currentMode = GameMode::Endless;
            g_SceneManager.RequestChange(SceneID::Game);
        } });

    // ---- 将来の追加枠（Coming Soon 表示。選択不可） ----
    // 実装するときは isLocked を解放条件（UnlockManager判定）に差し替え、
    // onDecide に対応する GameMode 設定と遷移を書く
    m_Items.push_back({
        "BOSS RUSH", "Fight all bosses in a row",
        []() { return true; }, "Coming Soon",
        nullptr });

    m_Items.push_back({
        "CHALLENGE", "Special missions with unique rules",
        []() { return true; }, "Coming Soon",
        nullptr });

    // 拠点へ戻る
    m_Items.push_back({
        "BACK", "Return to the main menu",
        nullptr, nullptr,
        []() { g_SceneManager.RequestChange(SceneID::MainMenu); } });
}

void ExtraScene::Update(float dt)
{
    const int count = static_cast<int>(m_Items.size());

    if (Input::GetKeyTrigger(VK_UP) || Input::GetKeyTrigger('W'))
        m_Selected = (m_Selected - 1 + count) % count;
    if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
        m_Selected = (m_Selected + 1) % count;

    // ESC で拠点へ戻る
    if (Input::GetKeyTrigger(VK_ESCAPE))
    {
        g_SceneManager.RequestChange(SceneID::MainMenu);
        return;
    }

    // マウスホバー（Draw と同じ座標系）
    ImVec2      mousePos = ImGui::GetIO().MousePos;
    const float itemH    = 56.0f;
    const float startY   = SCREEN_HEIGHT * 0.32f;

    for (int i = 0; i < count; i++)
    {
        float y = startY + itemH * i;
        if (mousePos.y >= y && mousePos.y < y + itemH)
            m_Selected = i;
    }

    // 決定。ロック中（Coming Soon）の項目は受け付けない
    bool decide = Input::GetKeyTrigger(VK_RETURN) || ImGui::GetIO().MouseClicked[0];
    if (decide)
    {
        const ExtraItem& item = m_Items[m_Selected];
        const bool locked = item.isLocked && item.isLocked();
        if (!locked && item.onDecide)
            item.onDecide();
    }
}

void ExtraScene::Draw()
{
    Renderer::Begin();

    ImDrawList* dl   = ImGui::GetBackgroundDrawList();
    ImFont*     font = ImGui::GetFont();

    // 背景（EXTRAらしく赤みがかった暗色にする）
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(20, 8, 10, 255));

    // 見出し
    {
        const char* title   = "EXTRA";
        const float titleSz = 48.0f;
        ImVec2 ts = font->CalcTextSizeA(titleSz, FLT_MAX, 0.0f, title);
        float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;
        float  ty = SCREEN_HEIGHT * 0.14f;
        dl->AddText(font, titleSz, ImVec2(tx + 3, ty + 3), IM_COL32(0, 0, 0, 200), title);
        dl->AddText(font, titleSz, ImVec2(tx, ty), IM_COL32(255, 90, 70, 255), title);
    }

    // モード一覧
    const int   count  = static_cast<int>(m_Items.size());
    const float itemH  = 56.0f;
    const float itemSz = 32.0f;
    const float startY = SCREEN_HEIGHT * 0.32f;

    for (int i = 0; i < count; i++)
    {
        const ExtraItem& item   = m_Items[i];
        const bool       locked = item.isLocked && item.isLocked();
        const bool       sel    = (i == m_Selected);

        float y  = startY + itemH * i;
        float sz = sel ? itemSz + 4.0f : itemSz;

        ImU32 col = locked ? IM_COL32(90, 90, 90, 200)
                  : sel    ? IM_COL32(255, 90, 70, 255)
                           : IM_COL32(180, 180, 180, 200);

        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, item.label);
        float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;

        if (sel)
            dl->AddText(font, sz - 4.0f, ImVec2(tx - 28.0f, y + 2.0f),
                IM_COL32(255, 90, 70, 255), ">");

        dl->AddText(font, sz, ImVec2(tx + 2, y + 2), IM_COL32(0, 0, 0, 160), item.label);
        dl->AddText(font, sz, ImVec2(tx, y), col, item.label);

        if (locked)
        {
            char lockText[96];
            sprintf_s(lockText, "[LOCKED] %s", item.lockedHint ? item.lockedHint : "");
            dl->AddText(font, 18.0f, ImVec2(tx + ts.x + 20.0f, y + sz * 0.5f - 9.0f),
                IM_COL32(200, 120, 120, 220), lockText);
        }
    }

    // 選択中項目の説明文（画面下部）
    {
        const ExtraItem& item = m_Items[m_Selected];
        const bool locked = item.isLocked && item.isLocked();
        const char* desc = (locked && item.lockedHint) ? item.lockedHint : item.description;
        if (desc)
        {
            ImVec2 ds = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, desc);
            float  dx = (SCREEN_WIDTH - ds.x) * 0.5f;
            dl->AddText(font, 20.0f, ImVec2(dx, SCREEN_HEIGHT * 0.86f),
                IM_COL32(180, 150, 150, 220), desc);
        }
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_TransitionManager.Draw();

    Renderer::End();
}
