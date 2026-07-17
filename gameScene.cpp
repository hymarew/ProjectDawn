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
#include "worldItem.h"
#include "worldItemPool.h"
#include "dropManager.h"
#include "damageEffectManager.h"
#include "dynamicLightManager.h"
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
#include "weatherManager.h"
#include "spaceRiftDebug.h"

#include <random>
#include "playerLog.h"
#include <ctime>
#include "gameContext.h"
#include "achievementManager.h"
#include "saveManager.h"

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

    g_BulletPool.Init(1000);
    g_BulletManager.Init();
    ParticleManager::GetInstance().Init();
    WeatherManager::GetInstance().Init();
    g_EnemyProjectilePool.Init();

    // ---- 武器収集・アイテムドロップシステム ----
    // GameContext::Instance() の初回呼び出しで各 Database の JSON ロードが走る。
    // Player::Init が装備復元で WeaponFactory を使うため、Player 生成より先に行う。
    auto& itemCtx = GameContext::Instance();
    g_WorldItemPool.Init(GameConfig::WorldItem::POOL_SIZE);
    g_DropManager.Init(&itemCtx.dropDB, &itemCtx.itemFactory);
    m_PendingWeapons.Discard();  // 前ステージの残りが万一あっても空から始める
    m_PickupSystem.Init(&itemCtx.itemDB, &itemCtx.inventory, &m_PendingWeapons);
    m_PickupNotify.Init();   // EventBus へ Subscribe
    m_PickupEffect.Init();   // EventBus へ Subscribe

    Player* player = Manager::AddGameObject<Player>();

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

    // 武器使用率: Player の装備スロットから発射回数を収集
    Player* player = Manager::GetGameObject<Player>();
    if (player)
    {
        for (int i = 0; i < (int)EquipSlot::Count; i++)
        {
            if (Weapon* w = player->GetEquip().GetWeapon((EquipSlot)i))
                g_PlayerLog.weaponShotsFired[w->GetName()] += w->GetFireCount();
        }
    }

    g_PlayerLog.Save();

    // 今回プレイ分のキル数を累計統計へ加算して保存する（実績のキル系判定に使う）
    const int runKills = g_EnemyPool.GetKillCount();
    if (runKills > 0)
    {
        g_SaveManager.AddStat("totalKills", runKills);
        g_SaveManager.Save();
    }

    m_WaveManager.Uninit();

    // ---- 武器収集・アイテムドロップシステムの後始末 ----
    // 仮取得武器を破棄する。クリア時は Update 内の CommitTo で既に空のため無害。
    // ゲームオーバー・リタイア・タイトル戻りは全てここを通るので、破棄漏れがない
    m_PendingWeapons.Discard();

    m_PickupNotify.Uninit();  // EventBus から Unsubscribe
    m_PickupEffect.Uninit();
    g_DropManager.Uninit();

    auto& list = Manager::GetGameObjectList();
    for (GameObject* obj : list) { obj->Uninit(); delete obj; }
    list.clear();
    Manager::SetCamera(nullptr);
    Mouse::SetLocked(false);

    g_BulletPool.Uninit();
    g_BulletManager.Uninit();
    ParticleManager::GetInstance().Uninit();
    WeatherManager::GetInstance().Uninit();
    g_EnemyPool.Uninit();
    g_EnemyProjectilePool.Uninit();
    g_WorldItemPool.Uninit();

    // ModelRenderer::UnloadAll() が MODEL* を全て破棄するため、
    // それらを static キャッシュしている各クラスも合わせてリセットする。
    // これを怠ると次回 Init() 時に m_IsLoaded==true のまま解放済みポインタを使い続けてしまう。
    Tree::ReleaseShaders();
    Scorpion::ReleaseShaders();
    Grass::ReleaseShaders();
    WorldItem::ReleaseShaders();

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

    // 3キー: パーティクルのCPU/GPUシミュレーション切替（比較デモ用）
    if (Input::GetKeyTrigger('3'))
    {
        auto& pm = ParticleManager::GetInstance();
        pm.SetUseGPU(!pm.IsUseGPU());
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
    // オプション画面表示中の ESC は OptionsMenu の「閉じる」操作なので、
    // ここでのポーズ解除は行わない（PauseMenu::Update 内で処理される）
    if (Input::GetKeyTrigger(VK_ESCAPE) && !m_PauseMenu.IsOptionsOpen())
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
    WeatherManager::GetInstance().Update(dt);
    g_DamageVisualizer.Update(dt);

    Player* player = Manager::GetGameObject<Player>();

    // ---- ワールドドロップの更新と取得判定 ----
    g_WorldItemPool.Update(dt);                    // 浮遊アニメ・寿命
    m_PickupSystem.Update(player, g_WorldItemPool); // 接触 → 取得 → イベント発行
    m_PickupNotify.Update(dt);                     // 取得トーストの時間経過

    g_CollisionManager.CheckEnemyVsPlayer    (g_EnemyPool, player);
    g_CollisionManager.CheckObstacleVsEnemies(g_EnemyPool);
    g_CollisionManager.Update();

    // 被弾UI演出（HUDシェイク・HPフラッシュ・赤ビネット・低HP心拍）を更新する
    g_DamageEffectManager.Update(dt, player ? (player->GetHp() / player->GetMaxHp()) : 1.0f);

    m_WaveManager.Update(dt);

    // ---- 実績 ----
    // 累計キル数 =（過去プレイの合計）+（今回プレイ分）。
    // 今回分の加算保存は Uninit で行うため、判定はここで足し合わせて行う
    g_AchievementManager.CheckKillMilestones(
        g_SaveManager.GetStat("totalKills") + g_EnemyPool.GetKillCount());
    g_AchievementManager.Update(dt);  // 解放トーストの時間経過

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
        auto& ctx = GameContext::Instance();

        // 実績: Legendary 武器の持ち帰り（正式取得の直前に仮取得リストで判定する）
        for (WeaponID id : m_PendingWeapons.GetAll())
        {
            const WeaponData* data = ctx.weaponDB.Find(id);
            if (data && data->rarity == Rarity::Legendary)
                g_AchievementManager.Unlock("legendary");
        }

        // 仮取得武器をここで正式取得する（クリア時のみ save.json に載る）。
        // この行を通らずにシーンが終わると Uninit の Discard で失われる
        m_PendingWeapons.CommitTo(ctx.inventory);

        // 実績: 全武器収集（正式取得後の所持数がマスタ全件に達したか）
        if (ctx.inventory.GetWeaponCount() >= (int)ctx.weaponDB.GetAll().size())
            g_AchievementManager.Unlock("collector");

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
    g_WorldItemPool.DrawShadow();
    g_ShadowRenderer->End();

    // Main Pass
    Renderer::Begin();

    // 動的ポイントライト（ロケットの噴射炎・爆発フラッシュ等）をGPUへ送る。
    // メインパスの描画開始前に1回呼べば、以降の全描画で参照される。
    g_DynamicLightManager.Apply();

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
            g_WorldItemPool.Draw();
        }
        if (layer == 2)
        {
            // パーティクルは半透明オブジェクト扱い（レイヤー2）で描画
            ParticleManager::GetInstance().Draw();

            // 天候（雨・雪）もパーティクルと同じ半透明レイヤーで描画する
            WeatherManager::GetInstance().Draw();
        }
    }

    // SpaceRift(空間の裂け目)デバッグデモ。ImGuiからSpawnした場合のみ描画される
    g_SpaceRiftDebug.Draw();

    DrawHUD();
    DrawWaveAnnouncement();

    // WeaponUI（右下）
    m_WeaponUI.Draw(Manager::GetGameObject<Player>());

    // アイテム取得トースト（左中段）
    m_PickupNotify.Draw();

    // 実績解放トースト（上部中央）
    g_AchievementManager.DrawToasts();

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

    // 被弾時にHUD全体へ加算する短いシェイクオフセット（0.15秒, ±4px, 減衰）
    const Vector2 shake = g_DamageEffectManager.GetHudShakeOffset();

    // シェイクオフセット込みの座標を作るヘルパー（矩形描画で使う）
    auto S = [&](float x, float y) { return ImVec2(x + shake.x, y + shake.y); };

    // 影付きテキスト描画ヘルパー（シェイクオフセットを自動で加算する）
    auto ShadowText = [&](float x, float y, float sz, ImU32 col, const char* text)
    {
        x += shake.x; y += shake.y;
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

        // 被弾直後はHPバーを赤く点滅させ、0.2秒かけて通常色へ線形補間で戻す
        float flashT = g_DamageEffectManager.GetHpFlashRatio();
        if (flashT > 0.0f)
        {
            auto LerpU8 = [](int a, int b, float t) { return (int)(a + (b - a) * t); };
            int r = LerpU8((barCol >> IM_COL32_R_SHIFT) & 0xFF, 255, flashT);
            int g = LerpU8((barCol >> IM_COL32_G_SHIFT) & 0xFF,  40, flashT);
            int b = LerpU8((barCol >> IM_COL32_B_SHIFT) & 0xFF,  40, flashT);
            barCol = IM_COL32(r, g, b, 230);
        }

        ShadowText(x, y, 15.0f, IM_COL32(200,200,200,200), "HP");

        // バー背景
        dl->AddRectFilled(S(x, y+18), S(x+barW, y+18+barH),
            IM_COL32(30,30,30,200), 3.0f);
        // バー本体
        if (ratio > 0.0f)
            dl->AddRectFilled(S(x+1, y+19), S(x+1+(barW-2)*ratio, y+18+barH-1),
                barCol, 3.0f);
        // 枠
        dl->AddRect(S(x, y+18), S(x+barW, y+18+barH),
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
            dl->AddRectFilled(S(barX, curY), S(barX+BAR_W, curY+BAR_H),
                IM_COL32(30,10,10,210), 4.0f);
            // 本体
            if (ratio > 0.0f)
                dl->AddRectFilled(S(barX+1, curY+1),
                    S(barX+1+(BAR_W-2)*ratio, curY+BAR_H-1), barCol, 4.0f);
            // 光沢ライン
            dl->AddRectFilled(S(barX+2, curY+2),
                S(barX+2+(BAR_W-4)*ratio, curY+5),
                IM_COL32(255,180,180,60), 2.0f);
            // 枠
            dl->AddRect(S(barX, curY), S(barX+BAR_W, curY+BAR_H),
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
        const float baseY = SCREEN_HEIGHT - MY - lineH * 4.0f;

        char buf[64];

        snprintf(buf, sizeof(buf), "Scorpion: %d", g_EnemyPool.GetActiveCount());
        ShadowText(x, baseY, sz, IM_COL32(180,220,255,220), buf);

        snprintf(buf, sizeof(buf), "Spawner : %d / %d",
            g_StageManager.GetAliveCount(), g_StageManager.GetTotalCount());
        ShadowText(x, baseY+lineH, sz, IM_COL32(210,210,210,220), buf);

        snprintf(buf, sizeof(buf), "Wave    : %d / %d",
            m_WaveManager.GetCurrentWave(), m_WaveManager.GetTotalWaves());
        ShadowText(x, baseY+lineH*2, sz, IM_COL32(210,210,210,220), buf);

        // 仮取得武器の数。クリアしないと失われるため、1本以上あるときは
        // 金色で目立たせて「持ち帰り待ち」であることを伝える
        snprintf(buf, sizeof(buf), "Found   : %d", m_PendingWeapons.GetCount());
        ShadowText(x, baseY+lineH*3, sz,
            (m_PendingWeapons.GetCount() > 0) ? IM_COL32(255,220,80,230)
                                              : IM_COL32(210,210,210,220), buf);
    }

    // ========================================================
    // 赤ビネット（画面端のみ。中央は透明。被弾フラッシュ + 低HP心拍）
    // ========================================================
    {
        float alpha = g_DamageEffectManager.GetVignetteAlpha();
        if (alpha > 0.0f)
        {
            const float size = GameConfig::DamageEffect::VIGNETTE_SIZE;
            ImU32 edgeCol   = IM_COL32(200, 20, 20, (int)(alpha * 255.0f));
            ImU32 transCol  = IM_COL32(200, 20, 20, 0);

            // 上端（上が濃く、下に向かって透明）
            dl->AddRectFilledMultiColor(
                ImVec2(0.0f, 0.0f), ImVec2((float)SCREEN_WIDTH, size),
                edgeCol, edgeCol, transCol, transCol);
            // 下端
            dl->AddRectFilledMultiColor(
                ImVec2(0.0f, (float)SCREEN_HEIGHT - size), ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
                transCol, transCol, edgeCol, edgeCol);
            // 左端
            dl->AddRectFilledMultiColor(
                ImVec2(0.0f, 0.0f), ImVec2(size, (float)SCREEN_HEIGHT),
                edgeCol, transCol, transCol, edgeCol);
            // 右端
            dl->AddRectFilledMultiColor(
                ImVec2((float)SCREEN_WIDTH - size, 0.0f), ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
                transCol, edgeCol, edgeCol, transCol);
        }
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
