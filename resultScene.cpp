#include "main.h"
#include "resultScene.h"
#include "sceneManager.h"
#include "stageManager.h"
#include "rankingManager.h"
#include "gameContext.h"
#include "transitionManager.h"
#include "renderer.h"
#include "input.h"
#include "playerLog.h"
#include "achievementManager.h"

// -------------------------------------------------------
// カーソル選択肢
// -------------------------------------------------------
static const char* s_Options[]   = { "Retry", "Stage Select" };
static constexpr int s_OptCount  = 2;

// -------------------------------------------------------
// Init : ランキング登録 & ステージ解放処理
// -------------------------------------------------------
void ResultScene::Init()
{
    ShowCursor(TRUE);
    m_Selected = 0;

    g_RankingManager.Load();

    if (!g_StageManager.IsGameOver())
    {
        // ランキング登録
        RankEntry entry;
        entry.playTime       = g_StageManager.GetResultPlayTime();
        entry.killCount      = g_StageManager.GetResultKillCount();
        entry.destroyedCount = g_StageManager.GetResultDestroyedCount();
        if (g_RankingManager.TryRegister(entry))
            g_RankingManager.Save();

        // クリアしたステージの次に位置するステージを解放する。
        // 次のステージが存在しない（=最終ステージ）場合は StoryComplete へ
        // 遷移するため、ここでは何も解放しない。
        StageID cleared = GameContext::Instance().currentStage;
        auto&   db      = GameContext::Instance().stageDB;

        if (const StageData* next = db.GetNextStage(cleared))
            db.UnlockStage(next->id);

        // ---- クリア時にのみ解放される実績 ----
        g_AchievementManager.Unlock("firstClear");

        // 最終ステージ（次のステージが存在しない）のクリアでストーリー完走
        if (db.GetNextStage(cleared) == nullptr)
            g_AchievementManager.Unlock("storyComplete");

        // 無被弾クリア（GameScene::Uninit で確定した PlayerLog を参照する）
        if (g_PlayerLog.totalDamageTaken <= 0.0f)
            g_AchievementManager.Unlock("noDamageClear");
    }
}

void ResultScene::Uninit()
{
}

// -------------------------------------------------------
// Update : カーソル操作 & 決定
// -------------------------------------------------------
void ResultScene::Update(float dt)
{
    // 実績解放トーストの時間経過（GameScene 中に解放された分もここで流れ続ける）
    g_AchievementManager.Update(dt);

    if (Input::GetKeyTrigger(VK_UP)   || Input::GetKeyTrigger('W'))
        m_Selected = (m_Selected - 1 + s_OptCount) % s_OptCount;
    if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
        m_Selected = (m_Selected + 1) % s_OptCount;

    // マウスホバー（Draw と同じ座標系）
    ImVec2      mp     = ImGui::GetIO().MousePos;
    const float optY   = SCREEN_HEIGHT * 0.82f;
    const float optH   = 44.0f;

    for (int i = 0; i < s_OptCount; i++)
    {
        float ry = optY + optH * i;
        if (mp.y >= ry && mp.y < ry + optH)
            m_Selected = i;
    }

    bool decide = Input::GetKeyTrigger(VK_RETURN) || ImGui::GetIO().MouseClicked[0];
    if (decide)
    {
        switch (m_Selected)
        {
        case 0: // Retry：同じステージをもう一度
            g_SceneManager.RequestChange(SceneID::Game);
            break;
        case 1:
        {
            // 最終ステージ（次のステージが存在しない）クリア時だけ
            // StoryComplete（エンディング演出）を挟んでから StageSelect へ戻る。
            // それ以外は直接 StageSelect へ。
            auto&   db          = GameContext::Instance().stageDB;
            StageID cleared     = GameContext::Instance().currentStage;
            bool    isLastStage = (db.GetNextStage(cleared) == nullptr);

            if (!g_StageManager.IsGameOver() && isLastStage)
            {
                g_SceneManager.RequestChange(SceneID::StoryComplete);
            }
            else
            {
                g_SceneManager.RequestChange(SceneID::StageSelect);
            }
            break;
        }
        }
    }
}

// -------------------------------------------------------
// ユーティリティ：時間を MM:SS.ss 文字列に変換
// -------------------------------------------------------
static void FormatTime(float seconds, char* buf, int bufSize)
{
    int totalSec = (int)seconds;
    int min      = totalSec / 60;
    int sec      = totalSec % 60;
    int cs       = (int)((seconds - (float)totalSec) * 100.0f);
    snprintf(buf, bufSize, "%02d:%02d.%02d", min, sec, cs);
}

// -------------------------------------------------------
// Draw
// -------------------------------------------------------
void ResultScene::Draw()
{
    Renderer::Begin();

    ImDrawList*  dl     = ImGui::GetBackgroundDrawList();
    ImFont*      font   = ImGui::GetFont();
    const bool   isOver = g_StageManager.IsGameOver();

    // ---- 背景 ----
    ImU32 bgCol = isOver
        ? IM_COL32(5,  0,  0,  255)
        : IM_COL32(0,  5,  10, 255);
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        bgCol);

    // ---- メインタイトル ----
    {
        const char* title = isOver ? "GAME OVER" : "STAGE CLEAR";
        const float sz    = 64.0f;
        ImU32       col   = isOver
            ? IM_COL32(220, 60,  60,  255)
            : IM_COL32(100, 220, 100, 255);

        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, title);
        float  tx = (SCREEN_WIDTH  - ts.x) * 0.5f;
        float  ty = SCREEN_HEIGHT  * 0.10f;
        dl->AddText(font, sz, ImVec2(tx + 3, ty + 3), IM_COL32(0, 0, 0, 200), title);
        dl->AddText(font, sz, ImVec2(tx, ty), col, title);
    }

    // ---- 統計 ----
    {
        const float sz    = 24.0f;
        const float lineH = sz + 8.0f;
        float       y     = SCREEN_HEIGHT * 0.28f;

        auto DrawStat = [&](const char* text, ImU32 col = IM_COL32(220, 220, 100, 255))
        {
            ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, text);
            float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;
            dl->AddText(font, sz, ImVec2(tx + 2, y + 2), IM_COL32(0, 0, 0, 160), text);
            dl->AddText(font, sz, ImVec2(tx, y), col, text);
            y += lineH;
        };

        char timeBuf[32];
        FormatTime(g_StageManager.GetResultPlayTime(), timeBuf, sizeof(timeBuf));
        char timeStr[64];
        snprintf(timeStr, sizeof(timeStr), "Time              : %s", timeBuf);
        DrawStat(timeStr, isOver
            ? IM_COL32(200, 200, 200, 255)
            : IM_COL32(120, 200, 255, 255));

        char spawnerStr[64];
        snprintf(spawnerStr, sizeof(spawnerStr),
            "Destroyed Spawner : %d", g_StageManager.GetResultDestroyedCount());
        DrawStat(spawnerStr);

        char killStr[64];
        snprintf(killStr, sizeof(killStr),
            "Killed Scorpion   : %d", g_StageManager.GetResultKillCount());
        DrawStat(killStr);
    }

    // ---- ランキング（STAGE CLEAR のみ） ----
    if (!isOver)
    {
        const auto& entries   = g_RankingManager.GetEntries();
        const int   newIndex  = g_RankingManager.GetNewEntryIndex();
        const float rankSz    = 22.0f;
        const float rankLineH = rankSz + 6.0f;

        const float panelW = 420.0f;
        const float panelH = rankLineH * (RankingManager::MAX_ENTRIES + 1) + 28.0f;
        const float panelX = (SCREEN_WIDTH  - panelW) * 0.5f;
        const float panelY = SCREEN_HEIGHT  * 0.50f;

        dl->AddRectFilled(
            ImVec2(panelX, panelY),
            ImVec2(panelX + panelW, panelY + panelH),
            IM_COL32(10, 15, 30, 200), 6.0f);
        dl->AddRect(
            ImVec2(panelX, panelY),
            ImVec2(panelX + panelW, panelY + panelH),
            IM_COL32(80, 120, 200, 160), 6.0f, 0, 1.0f);

        {
            const char* header = "- RANKING -";
            ImVec2 hs = font->CalcTextSizeA(rankSz, FLT_MAX, 0.0f, header);
            float  hx = panelX + (panelW - hs.x) * 0.5f;
            dl->AddText(font, rankSz, ImVec2(hx, panelY + 8.0f),
                IM_COL32(160, 190, 255, 220), header);
        }

        for (int i = 0; i < RankingManager::MAX_ENTRIES; i++)
        {
            float ey     = panelY + 8.0f + rankLineH * (i + 1);
            bool  isNew  = (i == newIndex);

            if (isNew)
                dl->AddRectFilled(
                    ImVec2(panelX + 4.0f, ey - 2.0f),
                    ImVec2(panelX + panelW - 4.0f, ey + rankSz + 2.0f),
                    IM_COL32(50, 100, 200, 100), 3.0f);

            ImU32 rankCol;
            switch (i)
            {
            case 0:  rankCol = IM_COL32(255, 215,   0, 255); break;
            case 1:  rankCol = IM_COL32(192, 192, 192, 255); break;
            case 2:  rankCol = IM_COL32(205, 127,  50, 255); break;
            default: rankCol = IM_COL32(180, 180, 180, 200); break;
            }

            ImU32 textCol = isNew ? IM_COL32(120, 200, 255, 255) : IM_COL32(220, 220, 220, 220);
            char  rankNumBuf[8];
            snprintf(rankNumBuf, sizeof(rankNumBuf), "#%d", i + 1);

            if (i < (int)entries.size())
            {
                char timeBuf[32];
                FormatTime(entries[i].playTime, timeBuf, sizeof(timeBuf));
                float lx = panelX + 14.0f;

                dl->AddText(font, rankSz, ImVec2(lx, ey), rankCol, rankNumBuf);

                char detailBuf[128];
                snprintf(detailBuf, sizeof(detailBuf),
                    "%s    Kill: %d", timeBuf, entries[i].killCount);
                dl->AddText(font, rankSz, ImVec2(lx + 46.0f, ey), textCol, detailBuf);

                if (isNew)
                    dl->AddText(font, rankSz - 4.0f,
                        ImVec2(panelX + panelW - 55.0f, ey + 3.0f),
                        IM_COL32(100, 220, 255, 255), "NEW!");
            }
            else
            {
                char emptyBuf[32];
                snprintf(emptyBuf, sizeof(emptyBuf), "%-3s  ---", rankNumBuf);
                dl->AddText(font, rankSz, ImVec2(panelX + 14.0f, ey),
                    IM_COL32(100, 100, 120, 160), emptyBuf);
            }
        }
    }

    // ---- 選択肢（Retry / Stage Select） ----
    {
        const float optSz = 28.0f;
        const float optY  = SCREEN_HEIGHT * 0.82f;
        const float optH  = 44.0f;

        for (int i = 0; i < s_OptCount; i++)
        {
            float  oy  = optY + optH * i;
            bool   sel = (i == m_Selected);
            ImU32  col = sel ? IM_COL32(255, 220, 50, 255) : IM_COL32(180, 180, 180, 200);
            float  sz  = sel ? optSz + 2.0f : optSz;

            if (sel)
            {
                ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, s_Options[i]);
                float  cx = (SCREEN_WIDTH - ts.x) * 0.5f;
                dl->AddText(font, sz, ImVec2(cx - 24.0f, oy),
                    IM_COL32(255, 220, 50, 255), ">");
            }

            ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, s_Options[i]);
            float  tx = (SCREEN_WIDTH - ts.x) * 0.5f;
            dl->AddText(font, sz, ImVec2(tx + 2, oy + 2), IM_COL32(0, 0, 0, 160), s_Options[i]);
            dl->AddText(font, sz, ImVec2(tx, oy), col, s_Options[i]);
        }
    }

    // 実績解放トースト（上部中央）
    g_AchievementManager.DrawToasts();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_TransitionManager.Draw();

    Renderer::End();
}
