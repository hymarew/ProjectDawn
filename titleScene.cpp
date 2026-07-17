#include "main.h"
#include "titleScene.h"
#include "sceneManager.h"
#include "transitionManager.h"
#include "renderer.h"
#include "input.h"

void TitleScene::Init()
{
    ShowCursor(TRUE); // カーソルを表示する
    m_SelectedIndex = 0;
}

void TitleScene::Uninit()
{
    // 解放するものなし
}

void TitleScene::Update(float dt)
{
    // オーバーレイ表示中はそちらへ入力を委譲する
    if (m_Options.IsOpen())      { m_Options.Update();      return; }
    if (m_Achievements.IsOpen()) { m_Achievements.Update(); return; }

    // カーソル移動（↑ W / ↓ S）
    if (Input::GetKeyTrigger(VK_UP)   || Input::GetKeyTrigger('W'))
        m_SelectedIndex = (m_SelectedIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
    if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
        m_SelectedIndex = (m_SelectedIndex + 1) % ITEM_COUNT;

    // 決定
    if (Input::GetKeyTrigger(VK_RETURN))
    {
        switch (m_SelectedIndex)
        {
        case 0: g_SceneManager.RequestChange(SceneID::Menu); break;
        case 1: m_Achievements.Open(); break;
        case 2: m_Options.Open();      break;
        }
    }
}

void TitleScene::Draw()
{
    Renderer::Begin(); // 画面をクリア

    ImDrawList* dl   = ImGui::GetBackgroundDrawList();
    ImFont*     font = ImGui::GetFont();

    // 黒背景
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(0, 0, 0, 255));

    // --- タイトル文字 ---
    const char* title    = "Project Down";
    const float titleSize = 60.0f;
    ImVec2 tSize = font->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, title);
    float  tx    = (SCREEN_WIDTH  - tSize.x) * 0.5f;
    float  ty    = SCREEN_HEIGHT  * 0.30f;
    // 影
    dl->AddText(font, titleSize, ImVec2(tx + 3, ty + 3),
        IM_COL32(0, 0, 0, 200), title);
    // 本体
    dl->AddText(font, titleSize, ImVec2(tx, ty),
        IM_COL32(255, 220, 50, 255), title);

    // --- メニュー（Start / Achievements / Options） ---
    const char* labels[] = { "Start", "Achievements", "Options" };
    const float itemSz    = 28.0f;
    const float itemStart = SCREEN_HEIGHT * 0.55f;
    const float itemStep  = 48.0f;

    for (int i = 0; i < ITEM_COUNT; i++)
    {
        const float iy       = itemStart + itemStep * i;
        const bool  selected = (i == m_SelectedIndex);
        const float sz       = selected ? itemSz + 2.0f : itemSz;

        ImU32 col = selected
            ? IM_COL32(255, 220, 50, 255)
            : IM_COL32(180, 180, 180, 200);

        ImVec2 ls = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, labels[i]);
        float  lx = (SCREEN_WIDTH - ls.x) * 0.5f;

        if (selected)
            dl->AddText(font, sz, ImVec2(lx - 30.0f, iy),
                IM_COL32(255, 220, 50, 255), ">");

        dl->AddText(font, sz, ImVec2(lx + 2, iy + 2), IM_COL32(0, 0, 0, 160), labels[i]);
        dl->AddText(font, sz, ImVec2(lx, iy), col, labels[i]);
    }

    // --- 操作ヒント ---
    {
        const char* hint = "W/S : Select    ENTER : Decide";
        const float sz   = 16.0f;
        ImVec2 hs = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, hint);
        dl->AddText(font, sz,
            ImVec2((SCREEN_WIDTH - hs.x) * 0.5f, SCREEN_HEIGHT * 0.88f),
            IM_COL32(140, 140, 150, 200), hint);
    }

    // --- オーバーレイ（表示中のみ） ---
    m_Options.Draw();
    m_Achievements.Draw();

    // ImGui の描画データを GPU に送る
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // トランジション（Fade等）オーバーレイ。全UIより手前に描画するためImGuiの後で呼ぶ
    g_TransitionManager.Draw();

    Renderer::End();
}
