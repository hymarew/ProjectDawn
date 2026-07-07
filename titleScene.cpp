#include "main.h"
#include "titleScene.h"
#include "sceneManager.h"
#include "fadeManager.h"
#include "renderer.h"
#include "input.h"

void TitleScene::Init()
{
    // タイトルでは特別な初期化不要
    ShowCursor(TRUE); // カーソルを表示する
}

void TitleScene::Uninit()
{
    // 解放するものなし
}

void TitleScene::Update(float dt)
{
    // Enter キーが押されたらゲームへ遷移
    // Title → Menu へ変更（以前は直接 Game へ遷移していた）
    if (Input::GetKeyTrigger(VK_RETURN))
        g_SceneManager.RequestChange(SceneID::Menu);
}

void TitleScene::Draw()
{
    Renderer::Begin(); // 画面をクリア

    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    // 黒背景
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(0, 0, 0, 255));

    // --- タイトル文字 ---
    const char* title    = "ALIEN SHOOTER";
    const float titleSize = 60.0f;
    ImVec2 tSize = ImGui::GetFont()->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, title);
    float  tx    = (SCREEN_WIDTH  - tSize.x) * 0.5f;
    float  ty    = SCREEN_HEIGHT  * 0.35f;
    // 影
    dl->AddText(ImGui::GetFont(), titleSize, ImVec2(tx + 3, ty + 3),
        IM_COL32(0, 0, 0, 200), title);
    // 本体
    dl->AddText(ImGui::GetFont(), titleSize, ImVec2(tx, ty),
        IM_COL32(255, 220, 50, 255), title);

    // --- Press Enter ---
    const char* prompt    = "Press  ENTER  to  Start";
    const float promptSize = 28.0f;
    ImVec2 pSize = ImGui::GetFont()->CalcTextSizeA(promptSize, FLT_MAX, 0.0f, prompt);
    float  px    = (SCREEN_WIDTH  - pSize.x) * 0.5f;
    float  py    = SCREEN_HEIGHT  * 0.62f;
    dl->AddText(ImGui::GetFont(), promptSize, ImVec2(px, py),
        IM_COL32(200, 200, 200, 255), prompt);

    // フェードオーバーレイ
    g_FadeManager.Draw();

    // ImGui の描画データを GPU に送る
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    Renderer::End();
}
