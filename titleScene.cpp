#include "main.h"
#include "titleScene.h"
#include "sceneManager.h"
#include "transitionManager.h"
#include "renderer.h"
#include "input.h"
#include "DirectXTex.h"

void TitleScene::Init()
{
    ShowCursor(TRUE); // カーソルを表示する
    m_SelectedIndex = 0;

    // ---- タイトルロゴの読み込み ----
    // 失敗しても m_LogoSRV が nullptr のままになり、Draw がテキスト表示へフォールバックする
    TexMetadata  metadata;
    ScratchImage image;
    if (SUCCEEDED(LoadFromWICFile(L"asset\\texture\\titleLogoDawn.png", WIC_FLAGS_NONE, &metadata, image)))
    {
        CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(),
            image.GetImageCount(), metadata, &m_LogoSRV);

        if (metadata.height > 0)
            m_LogoAspect = (float)metadata.width / (float)metadata.height;
    }
}

void TitleScene::Uninit()
{
    if (m_LogoSRV) { m_LogoSRV->Release(); m_LogoSRV = nullptr; }
}

void TitleScene::Update(float dt)
{
    // クレジット表示中はそちらへ入力を委譲する
    if (m_Credit.IsOpen()) { m_Credit.Update(); return; }

    // カーソル移動（↑ W / ↓ S）
    if (Input::GetKeyTrigger(VK_UP)   || Input::GetKeyTrigger('W'))
        m_SelectedIndex = (m_SelectedIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
    if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
        m_SelectedIndex = (m_SelectedIndex + 1) % ITEM_COUNT;

    // 決定（Start で拠点メニューへ。実績・設定は MainMenu 側にある）
    if (Input::GetKeyTrigger(VK_RETURN))
    {
        switch (m_SelectedIndex)
        {
        case 0: g_SceneManager.RequestChange(SceneID::MainMenu); break;
        case 1: m_Credit.Open(); break;
        case 2: PostQuitMessage(0); break;
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

    // --- タイトルロゴ ---
    if (m_LogoSRV)
    {
        // 画面幅の55%に収め、アスペクト比を保って中央上寄せに描く
        const float logoW   = SCREEN_WIDTH * 0.55f;
        const float logoH   = logoW / m_LogoAspect;
        const float centerX = SCREEN_WIDTH  * 0.5f;
        const float centerY = SCREEN_HEIGHT * 0.30f;

        dl->AddImage((ImTextureID)m_LogoSRV,
            ImVec2(centerX - logoW * 0.5f, centerY - logoH * 0.5f),
            ImVec2(centerX + logoW * 0.5f, centerY + logoH * 0.5f));
    }
    else
    {
        // ロゴの読み込みに失敗した場合のフォールバック（従来のテキスト表示）
        const char* title    = "Project Dawn";
        const float titleSize = 60.0f;
        ImVec2 tSize = font->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, title);
        float  tx    = (SCREEN_WIDTH  - tSize.x) * 0.5f;
        float  ty    = SCREEN_HEIGHT  * 0.30f;
        dl->AddText(font, titleSize, ImVec2(tx + 3, ty + 3),
            IM_COL32(0, 0, 0, 200), title);
        dl->AddText(font, titleSize, ImVec2(tx, ty),
            IM_COL32(255, 220, 50, 255), title);
    }

    // --- メニュー（Start / Credit / Exit） ---
    const char* labels[] = { "Start", "Credit", "Exit" };
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

    // --- クレジットオーバーレイ（表示中のみ） ---
    m_Credit.Draw();

    // ImGui の描画データを GPU に送る
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // トランジション（Fade等）オーバーレイ。全UIより手前に描画するためImGuiの後で呼ぶ
    g_TransitionManager.Draw();

    Renderer::End();
}
