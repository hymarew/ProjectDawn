#include "main.h"
#include "menuScene.h"
#include "sceneManager.h"
#include "gameContext.h"
#include "transitionManager.h"
#include "renderer.h"
#include "input.h"
#include "mouse.h"

static const char* s_Items[] = { "STORY", "ENDLESS", "EXIT" };
static constexpr int s_ItemCount = 3;

void MenuScene::Init()
{
    m_Selected = 0;
    ShowCursor(TRUE);
}

void MenuScene::Uninit()
{
}

void MenuScene::Update(float dt)
{
    // キーボード上下
    if (Input::GetKeyTrigger(VK_UP) || Input::GetKeyTrigger('W'))
        m_Selected = (m_Selected - 1 + s_ItemCount) % s_ItemCount;

    if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
        m_Selected = (m_Selected + 1) % s_ItemCount;

    // マウスホバー：Draw と同じ itemH / startY でヒット判定する
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    const float itemH   = 60.0f;
    const float startY  = SCREEN_HEIGHT * 0.40f;

    for (int i = 0; i < s_ItemCount; i++)
    {
        float y = startY + itemH * i;
        if (mousePos.y >= y && mousePos.y < y + itemH)
            m_Selected = i;
    }

    // 決定（Enter またはマウスクリック）
    // モードを GameContext に書き込んでから遷移することで
    // GameScene 側でモードを参照できるようにする
    bool decide = Input::GetKeyTrigger(VK_RETURN) || ImGui::GetIO().MouseClicked[0];
    if (decide)
    {
        switch (m_Selected)
        {
        case 0: // STORY
            GameContext::Instance().currentMode = GameMode::Story;
            // 武器選択 → ステージ選択 を経由してから GameScene へ進む
            g_SceneManager.RequestChange(SceneID::WeaponSelect);
            break;
        case 1: // ENDLESS
            GameContext::Instance().currentMode = GameMode::Endless;
            // 武器選択を経由してから GameScene へ進む（WeaponSelect がモードで分岐する）
            g_SceneManager.RequestChange(SceneID::WeaponSelect);
            break;
        case 2: // EXIT
            PostQuitMessage(0);
            break;
        }
    }
}

void MenuScene::Draw()
{
    Renderer::Begin();

    ImDrawList* dl   = ImGui::GetBackgroundDrawList();
    ImFont*     font = ImGui::GetFont();

    // 背景
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(0, 0, 0, 255));

    // タイトル
    {
        const char* title    = "ALIEN SHOOTER";
        const float titleSz  = 52.0f;
        ImVec2 ts = font->CalcTextSizeA(titleSz, FLT_MAX, 0.0f, title);
        float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;
        float  ty = SCREEN_HEIGHT * 0.18f;
        dl->AddText(font, titleSz, ImVec2(tx + 3, ty + 3), IM_COL32(0, 0, 0, 200), title);
        dl->AddText(font, titleSz, ImVec2(tx, ty), IM_COL32(255, 220, 50, 255), title);
    }

    // メニュー項目
    const float itemH  = 60.0f;
    const float itemSz = 36.0f;
    const float startY = SCREEN_HEIGHT * 0.40f;

    for (int i = 0; i < s_ItemCount; i++)
    {
        float  y      = startY + itemH * i;
        bool   sel    = (i == m_Selected);
        ImU32  col    = sel ? IM_COL32(255, 220, 50, 255) : IM_COL32(180, 180, 180, 200);
        float  sz     = sel ? itemSz + 4.0f : itemSz;  // 選択中は文字を少し大きく

        // 選択中は左側にマーカー（">" の位置はテキスト幅を基準に揃える）
        if (sel)
        {
            ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, s_Items[i]);
            float  cx = (SCREEN_WIDTH - ts.x) * 0.5f;
            dl->AddText(font, sz - 4.0f, ImVec2(cx - 28.0f, y + 2.0f),
                IM_COL32(255, 220, 50, 255), ">");
        }

        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, s_Items[i]);
        float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;
        dl->AddText(font, sz, ImVec2(tx + 2, y + 2), IM_COL32(0, 0, 0, 160), s_Items[i]);
        dl->AddText(font, sz, ImVec2(tx, y), col, s_Items[i]);
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_TransitionManager.Draw();

    Renderer::End();
}
