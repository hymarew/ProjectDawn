// ===================================================
// mainMenuScene.cpp
// ゲーム全体のハブ画面（拠点メニュー）の実装
// ===================================================

#include "main.h"
#include "mainMenuScene.h"
#include "sceneManager.h"
#include "gameContext.h"
#include "unlockManager.h"
#include "transitionManager.h"
#include "renderer.h"
#include "input.h"

void MainMenuScene::Init()
{
    m_Selected = 0;
    BuildItems();
    ShowCursor(TRUE);
}

void MainMenuScene::Uninit()
{
}

// ---------------------------------------------------------
// BuildItems : メニュー項目の一覧を構築する
// 新しい機能（ショップ・図鑑・武器強化など）はここに1エントリ足すだけでよい
// ---------------------------------------------------------
void MainMenuScene::BuildItems()
{
    m_Items.clear();

    // 出撃: ストーリーモードでステージ選択へ
    m_Items.push_back({
        "SORTIE", "Select a stage and head into battle",
        nullptr, nullptr,
        []() {
            GameContext::Instance().currentMode = GameMode::Story;
            g_SceneManager.RequestChange(SceneID::StageSelect);
        } });

    // 武器変更: 装備を変更して拠点へ戻る
    m_Items.push_back({
        "WEAPONS", "Change your equipped weapons",
        nullptr, nullptr,
        []() { g_SceneManager.RequestChange(SceneID::WeaponSelect); } });

    // EXTRA: 高難易度コンテンツ。ストーリークリアで解放される
    m_Items.push_back({
        "EXTRA", "High difficulty game modes",
        []() { return !UnlockManager::IsStoryCleared(); },
        "Clear Story to unlock",
        []() { g_SceneManager.RequestChange(SceneID::Extra); } });

    // 実績: オーバーレイで一覧表示（シーン遷移しない）
    m_Items.push_back({
        "ACHIEVEMENTS", "View your achievements",
        nullptr, nullptr,
        [this]() { m_Achievements.Open(); } });

    // 設定: オーバーレイで音量・感度などを変更（シーン遷移しない）
    m_Items.push_back({
        "OPTIONS", "Adjust volume and controls",
        nullptr, nullptr,
        [this]() { m_Options.Open(); } });

    // タイトルへ戻る
    m_Items.push_back({
        "BACK TO TITLE", "Return to the title screen",
        nullptr, nullptr,
        []() { g_SceneManager.RequestChange(SceneID::Title); } });
}

void MainMenuScene::Update(float dt)
{
    // オーバーレイ表示中はそちらへ入力を委譲する
    if (m_Options.IsOpen())      { m_Options.Update();      return; }
    if (m_Achievements.IsOpen()) { m_Achievements.Update(); return; }

    const int count = static_cast<int>(m_Items.size());

    // カーソル移動（↑W / ↓S）
    if (Input::GetKeyTrigger(VK_UP) || Input::GetKeyTrigger('W'))
        m_Selected = (m_Selected - 1 + count) % count;
    if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
        m_Selected = (m_Selected + 1) % count;

    // マウスホバー（Draw と同じ itemH / startY でヒット判定する）
    ImVec2      mousePos = ImGui::GetIO().MousePos;
    const float itemH    = 56.0f;
    const float startY   = SCREEN_HEIGHT * 0.30f;

    for (int i = 0; i < count; i++)
    {
        float y = startY + itemH * i;
        if (mousePos.y >= y && mousePos.y < y + itemH)
            m_Selected = i;
    }

    // 決定（Enter またはマウスクリック）。ロック中の項目は受け付けない
    bool decide = Input::GetKeyTrigger(VK_RETURN) || ImGui::GetIO().MouseClicked[0];
    if (decide)
    {
        const MenuItem& item = m_Items[m_Selected];
        const bool locked = item.isLocked && item.isLocked();
        if (!locked && item.onDecide)
            item.onDecide();
    }
}

void MainMenuScene::Draw()
{
    Renderer::Begin();

    ImDrawList* dl   = ImGui::GetBackgroundDrawList();
    ImFont*     font = ImGui::GetFont();

    // 背景
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(8, 10, 18, 255));

    // 見出し
    {
        const char* title   = "MAIN MENU";
        const float titleSz = 48.0f;
        ImVec2 ts = font->CalcTextSizeA(titleSz, FLT_MAX, 0.0f, title);
        float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;
        float  ty = SCREEN_HEIGHT * 0.14f;
        dl->AddText(font, titleSz, ImVec2(tx + 3, ty + 3), IM_COL32(0, 0, 0, 200), title);
        dl->AddText(font, titleSz, ImVec2(tx, ty), IM_COL32(255, 220, 50, 255), title);
    }

    // メニュー項目
    const int   count  = static_cast<int>(m_Items.size());
    const float itemH  = 56.0f;
    const float itemSz = 32.0f;
    const float startY = SCREEN_HEIGHT * 0.30f;

    for (int i = 0; i < count; i++)
    {
        const MenuItem& item   = m_Items[i];
        const bool      locked = item.isLocked && item.isLocked();
        const bool      sel    = (i == m_Selected);

        float y  = startY + itemH * i;
        float sz = sel ? itemSz + 4.0f : itemSz;

        // 色: ロック中は暗いグレー / 選択中は黄色 / 通常は薄いグレー
        ImU32 col = locked ? IM_COL32(90, 90, 90, 200)
                  : sel    ? IM_COL32(255, 220, 50, 255)
                           : IM_COL32(180, 180, 180, 200);

        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, item.label);
        float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;

        // 選択中は左側にマーカー
        if (sel)
            dl->AddText(font, sz - 4.0f, ImVec2(tx - 28.0f, y + 2.0f),
                IM_COL32(255, 220, 50, 255), ">");

        dl->AddText(font, sz, ImVec2(tx + 2, y + 2), IM_COL32(0, 0, 0, 160), item.label);
        dl->AddText(font, sz, ImVec2(tx, y), col, item.label);

        // ロック中は鍵マークと解放条件を右側に表示する
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
        const MenuItem& item = m_Items[m_Selected];
        const bool locked = item.isLocked && item.isLocked();
        const char* desc = (locked && item.lockedHint) ? item.lockedHint : item.description;
        if (desc)
        {
            ImVec2 ds = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, desc);
            float  dx = (SCREEN_WIDTH - ds.x) * 0.5f;
            dl->AddText(font, 20.0f, ImVec2(dx, SCREEN_HEIGHT * 0.86f),
                IM_COL32(150, 160, 180, 220), desc);
        }
    }

    // オーバーレイ（設定・実績）は最前面に描く
    m_Options.Draw();
    m_Achievements.Draw();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_TransitionManager.Draw();

    Renderer::End();
}
