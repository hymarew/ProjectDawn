//システム系
#include "main.h"
#include "gameScene.h"
#include "sceneManager.h"
#include "stageManager.h"
#include "waveManager.h"
#include "manager.h"
#include "renderer.h"
#include "shadowRenderer.h"
#include "camera.h"
#include "inputManager.h"
#include "input.h"
#include "mouse.h"
#include "GameConfig.h"
#include "colliderDebugRenderer.h"
#include "pauseMenu.h"
#include "Audio.h"

//オブジェクト系
#include "polygon2D.h"
#include "field.h"
#include "player.h"
#include "enemy.h"
#include "enemyPool.h"
#include "scorpion.h"
#include "modelRenderer.h"
#include "enemySpawner.h"
#include "bulletPool.h"
#include "bulletManager.h"
#include "enemyProjectilePool.h"
#include "weapon.h"
#include "collisionManager.h"
#include "damageVisualizer.h"
#include "tree.h"
#include "grass.h"
#include "sky.h"
#include "explosion.h"
#include "particleManager.h"

#include <random>
#include "playerLog.h"
#include <ctime>
#include "gameContext.h"

static std::string GetCurrentTimeString()
{
    time_t now = time(nullptr);
    struct tm t = {};
    localtime_s(&t, &now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    return buf;
}

// ゲームシーン開始 ─────────────────────────────────────
void GameScene::Init()
{
    ShowCursor(FALSE);
    Mouse::SetLocked(true);

    g_PlayerLog.Reset();
    m_StartTime = GetCurrentTimeString();
    g_PlayerLog.startTime = m_StartTime;

    Camera* camera = Manager::AddGameObject<Camera>();
    Manager::SetCamera(camera);
    camera->SetMode(CameraMode::TPS);

    Manager::AddGameObject<SKY>();
    Manager::AddGameObject<Field>();

    Player* player = Manager::AddGameObject<Player>();

    g_BulletPool.Init(1000);
    g_BulletManager.Init();
    ParticleManager::GetInstance().Init();
    g_EnemyProjectilePool.Init();

    g_EnemyPool.Init(GameConfig::Game::SCORPION_NUM);
    g_EnemyPool.ResetKillCount();

    g_StageManager.Reset();

    // Tree_05/08/10 × バリアントA/B/C からランダムに選択して配置
    static const TreeType TREE_TYPES[] = {
        TreeType::Tree05A, TreeType::Tree05B, TreeType::Tree05C,
        TreeType::Tree08A, TreeType::Tree08B, TreeType::Tree08C,
        TreeType::Tree10A, TreeType::Tree10B, TreeType::Tree10C,
    };
    for (int i = 0; i < 10; i++)
    {
        Vector3 pos = { (float)(rand() % 40 - 20), 0.0f, (float)(rand() % 40 - 20) };
        auto* tree = Manager::AddGameObject<Tree>();
        tree->SetPosition(pos);
        tree->SetTreeType(TREE_TYPES[rand() % 9]);
    }

    for (int i = 0; i < 30; i++)
    {
        Vector3 pos = { (float)(rand() % 60 - 30), 0.0f, (float)(rand() % 60 - 30) };
        Manager::AddGameObject<Grass>()->SetPosition(pos);
    }

    // Story モードのときだけ StageDatabase からステージデータを取得する。
    // Endless モードや取得失敗時は nullptr を渡し、WaveManager がフォールバック値を使う。
    auto& ctx = GameContext::Instance();
    if (ctx.currentMode == GameMode::Story)
        m_CurrentStage = ctx.stageDB.GetStage(ctx.currentStage);
    else
        m_CurrentStage = nullptr;

    m_WaveManager.Init(player, m_CurrentStage);
    m_WaveManager.StartFirstWave();
}

// ゲームシーン終了 ─────────────────────────────────────
void GameScene::Uninit()
{
    g_StageManager.Finalize(g_EnemyPool.GetKillCount(), m_PlayTime);

    // ---- ログデータを確定させてから保存 ----
    g_PlayerLog.endTime   = GetCurrentTimeString();
    g_PlayerLog.maxWave   = m_WaveManager.GetCurrentWave();
    g_PlayerLog.playTime  = m_PlayTime;
    g_PlayerLog.totalKills = g_EnemyPool.GetKillCount();
    g_PlayerLog.shotsFired = g_BulletPool.GetShotsFired();
    g_PlayerLog.shotsHit   = g_BulletPool.GetShotsHit();

    // クリア判定（deathCause 未設定 = まだ死んでいない = クリア）
    if (g_PlayerLog.deathCause.empty())
        g_PlayerLog.deathCause = "Clear";

    // 武器使用率: Player の各武器から発射回数を収集
    Player* player = Manager::GetGameObject<Player>();
    if (player)
    {
        for (Weapon* w : player->GetWeapons())
            g_PlayerLog.weaponShotsFired[w->GetName()] += w->GetFireCount();
    }

    g_PlayerLog.Save();

    m_WaveManager.Uninit();

    auto& list = Manager::GetGameObjectList();
    for (GameObject* obj : list) { obj->Uninit(); delete obj; }
    list.clear();
    Manager::SetCamera(nullptr);
    Mouse::SetLocked(false);

    g_BulletPool.Uninit();
    g_BulletManager.Uninit();
    ParticleManager::GetInstance().Uninit();
    g_EnemyPool.Uninit();
    g_EnemyProjectilePool.Uninit();

    // ModelRenderer::UnloadAll() が MODEL* を全て破棄するため、
    // それらを static キャッシュしている各クラスも合わせてリセットする。
    // これを怠ると次回 Init() 時に m_IsLoaded==true のまま解放済みポインタを使い続けてしまう。
    Tree::ReleaseShaders();
    Scorpion::ReleaseShaders();
    Grass::ReleaseShaders();

    ModelRenderer::UnloadAll();
}

// 毎フレーム更新 ─────────────────────────────────────────
void GameScene::Update(float dt)
{
    if (Input::GetKeyTrigger('1'))
    {
        g_ShowDebugUI = !g_ShowDebugUI;
        ShowCursor(g_ShowDebugUI ? TRUE : FALSE);
    }

    // 2キー: 広報・スクショ用カーソル解放トグル
    if (Input::GetKeyTrigger('2'))
    {
        m_FreeCursor = !m_FreeCursor;
        ShowCursor(m_FreeCursor ? TRUE : FALSE);
        Mouse::SetLocked(!m_FreeCursor);
    }

    // ---- チュートリアルオーバーレイ（ゲーム開始時1回） ----
    if (m_Tutorial.IsActive())
    {
        // 任意キーまたはマウス左クリックで閉じる
        bool anyKey = false;
        for (int vk = 0x08; vk <= 0xFE; vk++)
        {
            if (GetAsyncKeyState(vk) & 0x0001) { anyKey = true; break; }
        }
        if (anyKey) m_Tutorial.Close();

        return; // チュートリアル表示中はゲーム更新を完全停止
    }

    // ---- ESC でポーズトグル ----
    if (Input::GetKeyTrigger(VK_ESCAPE))
    {
        m_IsPaused = !m_IsPaused;
        if (m_IsPaused)
        {
            m_PauseMenu.Open();
            ShowCursor(TRUE);
            Mouse::SetLocked(false);
        }
        else
        {
            m_PauseMenu.Close();
            ShowCursor(FALSE);
            Mouse::SetLocked(true);
        }
    }

    // ---- ポーズ中はメニューのみ更新 ----
    if (m_IsPaused)
    {
        m_PauseMenu.Update();

        if (m_PauseMenu.IsResumeRequested())
        {
            m_IsPaused = false;
            m_PauseMenu.Close();
            ShowCursor(FALSE);
            Mouse::SetLocked(true);
        }
        else if (m_PauseMenu.IsRetryRequested())
        {
            g_SceneManager.RequestChange(SceneID::Game);
        }
        else if (m_PauseMenu.IsTitleRequested())
        {
            g_SceneManager.RequestChange(SceneID::Title);
        }
        else if (m_PauseMenu.IsExitRequested())
        {
            PostQuitMessage(0);
        }
        return;
    }

    // ---- 通常更新（ポーズ中はここに来ない） ----
    m_PlayTime += dt; // ポーズ中は return しているのでここには到達しない

    for (GameObject* obj : Manager::GetGameObjectList())
        obj->Update(dt);

    Manager::GetGameObjectList().remove_if([](GameObject* obj) { return obj->Destroy(); });

    g_EnemyPool.Update(dt);
    g_BulletPool.Update(dt, g_EnemyPool);
    g_EnemyProjectilePool.Update(dt, Manager::GetGameObject<Player>());
    ParticleManager::GetInstance().Update(dt);
    g_DamageVisualizer.Update(dt);

    Player* player = Manager::GetGameObject<Player>();
    g_CollisionManager.CheckEnemyVsPlayer    (g_EnemyPool, player);
    g_CollisionManager.CheckObstacleVsEnemies(g_EnemyPool);
    g_CollisionManager.Update();

    m_WaveManager.Update(dt);

    // ---- 終了判定 ----
    // player は衝突判定で既に取得済みのため再宣言不要
    player = Manager::GetGameObject<Player>();

    // プレイヤー死亡 → GAME OVER
    if (player && !player->IsAlive())
    {
        g_StageManager.SetGameOver();
        g_SceneManager.RequestChange(SceneID::Result);
    }
    // 全Wave クリア → STAGE CLEAR
    else if (m_WaveManager.IsAllWavesCleared())
    {
        g_SceneManager.RequestChange(SceneID::Result);
    }
}

// 毎フレーム描画 ─────────────────────────────────────────
void GameScene::Draw()
{
    // Shadow Pass
    g_ShadowRenderer->Begin();
    for (GameObject* obj : Manager::GetGameObjectList()) obj->DrawShadow();
    for (Enemy* e : g_EnemyPool.GetActiveEnemies()) e->DrawShadow();
    g_BulletManager.DrawShadow(g_BulletPool);
    g_ShadowRenderer->End();

    // Main Pass
    Renderer::Begin();
    Camera* camera = Manager::GetGameObject<Camera>();
    Vector3 forward = camera->GetForward();
    Vector3 position = camera->GetPosition();

    for (GameObject* gameObject : Manager::GetGameObjectList())
    {
        gameObject->CalcCameraZ(position, forward);
    }

    // Zソート //半透明オブジェクトだけでいい
    Manager::GetGameObjectList().sort([](GameObject* a, GameObject* b)
        {
            return a->GetCameraZ() > b->GetCameraZ();
        });

    
    g_ShadowRenderer->SetShadowMap();

    for (int layer = 0; layer < 4; layer++)
    {
        for (GameObject* obj : Manager::GetGameObjectList())
            if (obj->GetLayer() == layer) obj->Draw();

        if (layer == 1)
        {
            for (Enemy* e : g_EnemyPool.GetActiveEnemies()) e->Draw();
            g_BulletManager.Draw(g_BulletPool);
            g_EnemyProjectilePool.Draw();
        }
        if (layer == 2)
        {
            // パーティクルは半透明オブジェクト扱い（レイヤー2）で描画
            ParticleManager::GetInstance().Draw();
        }
    }

    DrawHUD();
    DrawWaveAnnouncement();

    // WeaponUI（右下）
    m_WeaponUI.Draw(Manager::GetGameObject<Player>());

    // ポーズオーバーレイ（ポーズ中のみ）
    if (m_IsPaused)
        m_PauseMenu.Draw();

    // チュートリアルオーバーレイ（ゲーム開始直後）
    if (m_Tutorial.IsActive())
        m_Tutorial.Draw();

    // デバッグUI・クロスヘア・ダメージ表示・フェード（Manager::ImGuiDraw 内部で描画）
    Manager::ImGuiDraw();

    Renderer::End();
}

// ゲームHUD ───────────────────────────────────────────────
void GameScene::DrawHUD()
{
    ImDrawList* dl    = ImGui::GetBackgroundDrawList();
    ImFont*     font  = ImGui::GetFont();
    const float MX    = 20.0f;  // 左右マージン
    const float MY    = 16.0f;  // 上マージン

    // 影付きテキスト描画ヘルパー
    auto ShadowText = [&](float x, float y, float sz, ImU32 col, const char* text)
    {
        dl->AddText(font, sz, ImVec2(x + 1.5f, y + 1.5f), IM_COL32(0,0,0,180), text);
        dl->AddText(font, sz, ImVec2(x, y), col, text);
    };
    // 中央揃えテキストヘルパー
    auto CenterText = [&](float cx, float y, float sz, ImU32 col, const char* text)
    {
        float w = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, text).x;
        ShadowText(cx - w * 0.5f, y, sz, col, text);
    };

    // ========================================================
    // 左上 : プレイヤー HP
    // ========================================================
    Player* player = Manager::GetGameObject<Player>();
    if (player)
    {
        const float barW  = 220.0f;
        const float barH  = 16.0f;
        const float x     = MX;
        const float y     = MY;
        const float ratio = player->GetHp() / player->GetMaxHp();

        ImU32 barCol = (ratio > 0.5f) ? IM_COL32(50, 200, 50, 230)
                     : (ratio > 0.25f)? IM_COL32(220, 160, 40, 230)
                                      : IM_COL32(220, 50,  50, 230);

        ShadowText(x, y, 15.0f, IM_COL32(200,200,200,200), "HP");

        // バー背景
        dl->AddRectFilled(ImVec2(x, y+18), ImVec2(x+barW, y+18+barH),
            IM_COL32(30,30,30,200), 3.0f);
        // バー本体
        if (ratio > 0.0f)
            dl->AddRectFilled(ImVec2(x+1, y+19), ImVec2(x+1+(barW-2)*ratio, y+18+barH-1),
                barCol, 3.0f);
        // 枠
        dl->AddRect(ImVec2(x, y+18), ImVec2(x+barW, y+18+barH),
            IM_COL32(180,180,180,160), 3.0f, 0, 1.2f);

        // 数値
        char buf[32];
        snprintf(buf, sizeof(buf), "%.0f / %.0f", player->GetHp(), player->GetMaxHp());
        ShadowText(x, y+18+barH+4, 14.0f, IM_COL32(210,210,210,220), buf);
    }

    // ========================================================
    // 上部中央 : スポナーごとの独立 HP ゲージ
    // ========================================================
    {
        const auto& spawners = g_StageManager.GetSpawners();

        const float BAR_W    = 400.0f;
        const float BAR_H    = 18.0f;
        const float LABEL_SZ = 14.0f;
        const float VAL_SZ   = 13.0f;
        const float ENTRY_H  = LABEL_SZ + BAR_H + VAL_SZ + 10.0f; // 1スポナー分の高さ
        const float cx       = SCREEN_WIDTH * 0.5f;
        const float barX     = cx - BAR_W * 0.5f;
        float       curY     = MY;

        int slot = 0; // 生存スポナーの通し番号（A, B, C …）
        for (auto* s : spawners)
        {
            if (s->IsDestroyed()) continue;

            const float ratio = s->GetMaxHp() > 0.0f
                              ? s->GetHp() / s->GetMaxHp() : 0.0f;

            // ---- ラベル "SPAWNER A / B / C …" ----
            char label[32];
            snprintf(label, sizeof(label), "SPAWNER %c", 'A' + slot);
            CenterText(cx, curY, LABEL_SZ, IM_COL32(200,200,200,200), label);
            curY += LABEL_SZ + 3.0f;

            // ---- HP バー ----
            ImU32 barCol = (ratio > 0.5f) ? IM_COL32(220, 60,  60, 230)
                         : (ratio > 0.25f)? IM_COL32(220, 140, 40, 230)
                                          : IM_COL32(160,  40, 40, 230);

            // 背景
            dl->AddRectFilled(ImVec2(barX, curY), ImVec2(barX+BAR_W, curY+BAR_H),
                IM_COL32(30,10,10,210), 4.0f);
            // 本体
            if (ratio > 0.0f)
                dl->AddRectFilled(ImVec2(barX+1, curY+1),
                    ImVec2(barX+1+(BAR_W-2)*ratio, curY+BAR_H-1), barCol, 4.0f);
            // 光沢ライン
            dl->AddRectFilled(ImVec2(barX+2, curY+2),
                ImVec2(barX+2+(BAR_W-4)*ratio, curY+5),
                IM_COL32(255,180,180,60), 2.0f);
            // 枠
            dl->AddRect(ImVec2(barX, curY), ImVec2(barX+BAR_W, curY+BAR_H),
                IM_COL32(200,100,100,200), 4.0f, 0, 1.5f);
            curY += BAR_H + 3.0f;

            // ---- HP 数値 ----
            char valBuf[32];
            snprintf(valBuf, sizeof(valBuf), "%.0f / %.0f", s->GetHp(), s->GetMaxHp());
            CenterText(cx, curY, VAL_SZ, IM_COL32(210,180,180,220), valBuf);
            curY += VAL_SZ + 10.0f; // 次スポナーとの間隔

            ++slot;
        }
    }

    // ========================================================
    // 右上 : タイム
    // ========================================================
    {
        int  totalSec = (int)m_PlayTime;
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", totalSec/60, totalSec%60);

        float tw = font->CalcTextSizeA(32.0f, FLT_MAX, 0.0f, timeBuf).x;
        float tx = SCREEN_WIDTH - MX - tw;
        ShadowText(tx, MY, 32.0f, IM_COL32(220,220,255,230), timeBuf);
    }

    // ========================================================
    // 左下 : Enemy / Spawner / Wave
    // ========================================================
    {
        const float sz    = 18.0f;
        const float lineH = sz + 6.0f;
        const float x     = MX;
        const float baseY = SCREEN_HEIGHT - MY - lineH * 3.0f;

        char buf[64];

        snprintf(buf, sizeof(buf), "Scorpion: %d", g_EnemyPool.GetActiveCount());
        ShadowText(x, baseY, sz, IM_COL32(180,220,255,220), buf);

        snprintf(buf, sizeof(buf), "Spawner : %d / %d",
            g_StageManager.GetAliveCount(), g_StageManager.GetTotalCount());
        ShadowText(x, baseY+lineH, sz, IM_COL32(210,210,210,220), buf);

        snprintf(buf, sizeof(buf), "Wave    : %d / %d",
            m_WaveManager.GetCurrentWave(), m_WaveManager.GetTotalWaves());
        ShadowText(x, baseY+lineH*2, sz, IM_COL32(210,210,210,220), buf);
    }
}

// Wave アナウンス演出（中央大文字） ────────────────────────
void GameScene::DrawWaveAnnouncement()
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    // "WAVE X" アナウンス中
    if (m_WaveManager.IsAnnouncing())
    {
        float alpha = m_WaveManager.GetAnnounceAlpha();
        if (alpha <= 0.0f) return;

        const char* text = m_WaveManager.GetAnnounceText();
        const float sz   = 80.0f;
        int a = (int)(alpha * 255.0f);

        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(sz, FLT_MAX, 0.0f, text);
        float tx = (SCREEN_WIDTH  - ts.x) * 0.5f;
        float ty = (SCREEN_HEIGHT - ts.y) * 0.42f;

        dl->AddText(ImGui::GetFont(), sz, ImVec2(tx + 4, ty + 4),
            IM_COL32(0, 0, 0, a / 2), text);
        dl->AddText(ImGui::GetFont(), sz, ImVec2(tx, ty),
            IM_COL32(255, 255, 255, a), text);
        return;
    }

    // "WAVE CLEAR" 次Wave待機中
    if (m_WaveManager.IsWaitingNextWave())
    {
        float progress = m_WaveManager.GetWaitProgress();

        // フェードイン・アウト（最初と最後の 25% でフェード）
        float alpha;
        if      (progress < 0.25f) alpha = progress / 0.25f;
        else if (progress > 0.75f) alpha = (1.0f - progress) / 0.25f;
        else                       alpha = 1.0f;

        if (alpha <= 0.0f) return;

        const char* text = "WAVE CLEAR";
        const float sz   = 64.0f;
        int a = (int)(alpha * 255.0f);

        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(sz, FLT_MAX, 0.0f, text);
        float tx = (SCREEN_WIDTH  - ts.x) * 0.5f;
        float ty = (SCREEN_HEIGHT - ts.y) * 0.42f;

        dl->AddText(ImGui::GetFont(), sz, ImVec2(tx + 3, ty + 3),
            IM_COL32(0, 0, 0, a / 2), text);
        dl->AddText(ImGui::GetFont(), sz, ImVec2(tx, ty),
            IM_COL32(100, 220, 255, a), text);  // 水色
    }
}
