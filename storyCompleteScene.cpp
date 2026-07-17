#include "main.h"
#include "storyCompleteScene.h"
#include "sceneManager.h"
#include "transitionManager.h"
#include "unlockManager.h"
#include "renderer.h"
#include "input.h"

void StoryCompleteScene::Init()
{
    ShowCursor(TRUE);

    // ストーリークリアを記録する（EXTRAコンテンツの解放条件。即セーブされる）
    UnlockManager::MarkStoryCleared();
}

void StoryCompleteScene::Uninit()
{
}

void StoryCompleteScene::Update(float dt)
{
    // 任意キーまたはマウスクリックで拠点（MainMenu）へ戻る。
    // エンディング画面なので ESC も含めて「どのキーでも先へ進める」仕様にしている。
    // 戻った拠点では EXTRA が解放されている（Init で記録済み）
    bool anyKey = ImGui::GetIO().MouseClicked[0]
               || Input::GetKeyTrigger(VK_RETURN)
               || Input::GetKeyTrigger(VK_SPACE)
               || Input::GetKeyTrigger(VK_ESCAPE);

    if (anyKey)
        g_SceneManager.RequestChange(SceneID::MainMenu);
}

void StoryCompleteScene::Draw()
{
    Renderer::Begin();

    ImDrawList* dl   = ImGui::GetBackgroundDrawList();
    ImFont*     font = ImGui::GetFont();

    // 背景（深い紺色でエンディング感を演出）
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(0, 0, 20, 255));

    // ---- 装飾ライン ----
    const float lineY1 = SCREEN_HEIGHT * 0.30f;
    const float lineY2 = SCREEN_HEIGHT * 0.58f;
    const float lineX1 = SCREEN_WIDTH  * 0.20f;
    const float lineX2 = SCREEN_WIDTH  * 0.80f;
    dl->AddLine(ImVec2(lineX1, lineY1), ImVec2(lineX2, lineY1), IM_COL32(180, 160, 80, 180), 1.5f);
    dl->AddLine(ImVec2(lineX1, lineY2), ImVec2(lineX2, lineY2), IM_COL32(180, 160, 80, 180), 1.5f);

    // ---- STORY COMPLETE ----
    {
        const char* title = "STORY COMPLETE";
        const float sz    = 56.0f;
        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, title);
        float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;
        float  ty = SCREEN_HEIGHT * 0.36f;
        dl->AddText(font, sz, ImVec2(tx + 3, ty + 3), IM_COL32(0, 0, 0, 200), title);
        dl->AddText(font, sz, ImVec2(tx, ty), IM_COL32(255, 230, 80, 255), title);
    }

    // ---- Thank you ----
    {
        const char* msg = "Thank you for playing.";
        const float sz  = 26.0f;
        ImVec2 ms = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, msg);
        float  mx = (SCREEN_WIDTH - ms.x) * 0.5f;
        float  my = SCREEN_HEIGHT * 0.63f;
        dl->AddText(font, sz, ImVec2(mx, my), IM_COL32(200, 200, 200, 220), msg);
    }

    // ---- 続行ガイド ----
    {
        const char* guide  = "Press any key to continue";
        const float guideSz = 20.0f;
        ImVec2 gs = font->CalcTextSizeA(guideSz, FLT_MAX, 0.0f, guide);
        float  gx = (SCREEN_WIDTH - gs.x) * 0.5f;
        float  gy = SCREEN_HEIGHT * 0.88f;
        dl->AddText(font, guideSz, ImVec2(gx, gy),
            IM_COL32(130, 130, 150, 200), guide);
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_TransitionManager.Draw();

    Renderer::End();
}
