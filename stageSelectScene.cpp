#include "main.h"
#include "stageSelectScene.h"
#include "sceneManager.h"
#include "gameContext.h"
#include "fadeManager.h"
#include "renderer.h"
#include "input.h"
#include "DirectXTex.h"

// 解放済みステージだけを抽出したリストを返す。
// Update と Draw の両方から毎フレーム呼ばれるが、ステージ数が少ないため問題ない。
// ResultScene での UnlockStage() 呼び出しが即座に反映される利点がある。
static std::vector<const StageData*> GetUnlockedStages()
{
    std::vector<const StageData*> result;
    for (const auto& s : GameContext::Instance().stageDB.GetStages())
    {
        if (s.unlocked)
            result.push_back(&s);
    }
    return result;
}

void StageSelectScene::Init()
{
    m_Selected = 0;
    ShowCursor(TRUE);

    // 全ステージのサムネイルをここで読み込んでおく（未解放のステージ分も含む）
    for (const auto& stage : GameContext::Instance().stageDB.GetStages())
    {
        if (stage.thumbnailPath.empty()) continue;

        std::wstring wpath(stage.thumbnailPath.begin(), stage.thumbnailPath.end());

        TexMetadata  metadata;
        ScratchImage image;
        if (FAILED(LoadFromWICFile(wpath.c_str(), WIC_FLAGS_NONE, &metadata, image)))
            continue;

        ID3D11ShaderResourceView* srv = nullptr;
        CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(),
            image.GetImageCount(), metadata, &srv);

        if (srv) m_Thumbnails[stage.id.number] = srv;
    }
}

void StageSelectScene::Uninit()
{
    for (auto& pair : m_Thumbnails)
        if (pair.second) pair.second->Release();
    m_Thumbnails.clear();
}

void StageSelectScene::Update(float dt)
{
    auto unlocked = GetUnlockedStages();
    const int count = (int)unlocked.size();
    if (count == 0) return;

    if (Input::GetKeyTrigger(VK_UP) || Input::GetKeyTrigger('W'))
        m_Selected = (m_Selected - 1 + count) % count;

    if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
        m_Selected = (m_Selected + 1) % count;

    // マウスホバー：Draw の listY / itemH と値を合わせておく必要がある。
    // X 座標は判定に含めず、Y 行全体をヒット領域にすることで
    // カーソルが行間に入ってもスルーされにくくしている。
    ImVec2      mousePos = ImGui::GetIO().MousePos;
    const float listY    = SCREEN_HEIGHT * 0.28f;
    const float itemH    = 48.0f;

    for (int i = 0; i < count; i++)
    {
        float rowY = listY + itemH * i;
        if (mousePos.y >= rowY && mousePos.y < rowY + itemH)
            m_Selected = i;
    }

    // 安全クランプ：解放数が減ることはないが念のため
    if (m_Selected >= count) m_Selected = count - 1;

    // 決定：GameContext にステージを書き込んでから GameScene へ遷移
    bool decide = Input::GetKeyTrigger(VK_RETURN) || ImGui::GetIO().MouseClicked[0];
    if (decide)
    {
        GameContext::Instance().currentStage = unlocked[m_Selected]->id;
        g_SceneManager.RequestChange(SceneID::Game);
    }
}

void StageSelectScene::Draw()
{
    Renderer::Begin();

    ImDrawList* dl    = ImGui::GetBackgroundDrawList();
    ImFont*     font  = ImGui::GetFont();
    auto        unlocked = GetUnlockedStages();
    const int   count    = (int)unlocked.size();

    // 背景
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(5, 5, 15, 255));

    // ---- 見出し ----
    {
        const char* title = "STAGE SELECT";
        const float sz    = 40.0f;
        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, title);
        float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;
        float  ty = SCREEN_HEIGHT * 0.10f;
        dl->AddText(font, sz, ImVec2(tx + 2, ty + 2), IM_COL32(0, 0, 0, 200), title);
        dl->AddText(font, sz, ImVec2(tx, ty), IM_COL32(255, 220, 50, 255), title);
    }

    // ---- ステージリスト（左ペイン、解放済みのみ） ----
    const float listX  = SCREEN_WIDTH  * 0.10f;
    const float listY  = SCREEN_HEIGHT * 0.28f;
    const float itemH  = 48.0f;
    const float itemSz = 28.0f;

    for (int i = 0; i < count; i++)
    {
        float  iy  = listY + itemH * i;
        bool   sel = (i == m_Selected);
        ImU32  col = sel ? IM_COL32(255, 220, 50, 255) : IM_COL32(180, 180, 180, 200);
        float  sz  = sel ? itemSz + 2.0f : itemSz;  // 選択行はわずかに大きく

        // カーソルマーカー（">" の X は listX 基準で固定オフセット）
        if (sel)
            dl->AddText(font, sz, ImVec2(listX - 22.0f, iy),
                IM_COL32(255, 220, 50, 255), ">");

        dl->AddText(font, sz, ImVec2(listX + 1, iy + 1), IM_COL32(0, 0, 0, 160),
            unlocked[i]->name.c_str());
        dl->AddText(font, sz, ImVec2(listX, iy), col, unlocked[i]->name.c_str());
    }

    // ---- サムネイルエリア ----
    {
        const float panelX = SCREEN_WIDTH  * 0.45f;
        const float panelY = SCREEN_HEIGHT * 0.26f;
        const float panelW = SCREEN_WIDTH  * 0.45f;
        const float panelH = SCREEN_HEIGHT * 0.30f;

        dl->AddRectFilled(
            ImVec2(panelX, panelY),
            ImVec2(panelX + panelW, panelY + panelH),
            IM_COL32(20, 20, 40, 220), 6.0f);

        ID3D11ShaderResourceView* thumbnail = nullptr;
        if (count > 0)
        {
            auto it = m_Thumbnails.find(unlocked[m_Selected]->id.number);
            if (it != m_Thumbnails.end())
                thumbnail = it->second;
        }

        if (thumbnail)
        {
            dl->AddImage((ImTextureID)thumbnail,
                ImVec2(panelX, panelY), ImVec2(panelX + panelW, panelY + panelH));
        }
        else
        {
            const char* dummy = "[ No Image ]";
            ImVec2 ds = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, dummy);
            dl->AddText(font, 20.0f,
                ImVec2(panelX + (panelW - ds.x) * 0.5f,
                       panelY + (panelH - 20.0f) * 0.5f),
                IM_COL32(80, 80, 120, 180), dummy);
        }

        dl->AddRect(
            ImVec2(panelX, panelY),
            ImVec2(panelX + panelW, panelY + panelH),
            IM_COL32(80, 100, 180, 160), 6.0f, 0, 1.5f);
    }

    // ---- 説明文エリア（選択中ステージ） ----
    if (count > 0)
    {
        const float descX  = SCREEN_WIDTH  * 0.45f;
        const float descY  = SCREEN_HEIGHT * 0.60f;
        const float descSz = 20.0f;

        const std::string& desc = unlocked[m_Selected]->description;

        // BackgroundDrawList の AddText は '\n' を改行として扱わないため
        // 手動で分割して行ごとに描画する
        float lineY = descY;
        std::string line;
        for (char c : desc)
        {
            if (c == '\n')
            {
                dl->AddText(font, descSz, ImVec2(descX, lineY),
                    IM_COL32(200, 200, 200, 220), line.c_str());
                lineY += descSz + 4.0f;
                line.clear();
            }
            else
            {
                line += c;
            }
        }
        if (!line.empty())
            dl->AddText(font, descSz, ImVec2(descX, lineY),
                IM_COL32(200, 200, 200, 220), line.c_str());
    }

    // ---- 操作ガイド ----
    {
        const char* guide   = "W/S  or  Arrow : Move      Enter / Click : Select";
        const float guideSz = 18.0f;
        ImVec2 gs = font->CalcTextSizeA(guideSz, FLT_MAX, 0.0f, guide);
        float  gx = (SCREEN_WIDTH - gs.x) * 0.5f;
        float  gy = SCREEN_HEIGHT * 0.93f;
        dl->AddText(font, guideSz, ImVec2(gx, gy),
            IM_COL32(120, 120, 140, 180), guide);
    }

    g_FadeManager.Draw();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    Renderer::End();
}
