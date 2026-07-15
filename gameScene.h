#pragma once
#include "scene.h"
#include "waveManager.h"
#include "weaponUI.h"
#include "pauseMenu.h"
#include "tutorialOverlay.h"
#include "stageData.h"
#include "itemPickupSystem.h"
#include "pickupNotificationUI.h"
#include "pickupEffectPlayer.h"
#include "pendingWeapons.h"
#include <string>

// =====================================================
// GameScene : ゲームプレイ画面
//   - WaveManager で Wave 進行を管理する
//   - プレイヤー死亡時は GAME OVER として ResultScene へ遷移
//   - 全 Wave クリアで STAGE CLEAR として ResultScene へ遷移
//   - ESC でポーズ（m_IsPaused）。ポーズ中はゲーム更新を停止する
// =====================================================
class GameScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;

private:
    void DrawHUD();              // Wave数・Spawner数・Enemy数・プレイヤーHP の常時表示
    void DrawWaveAnnouncement(); // "WAVE X" / "WAVE CLEAR" の大文字演出

    WaveManager     m_WaveManager;
    WeaponUI        m_WeaponUI;
    PauseMenu       m_PauseMenu;
    TutorialOverlay m_Tutorial;

    // ---- 武器収集・アイテムドロップシステム ----
    ItemPickupSystem     m_PickupSystem;   // 接触判定 → 取得処理 → イベント発行
    PickupNotificationUI m_PickupNotify;   // 取得トースト（EventBus リスナー）
    PickupEffectPlayer   m_PickupEffect;   // 取得エフェクト（EventBus リスナー）

    // ステージ中の仮取得武器。クリアで Inventory へ正式取得、
    // ゲームオーバー・途中終了では Uninit の Discard で失われる
    PendingWeapons       m_PendingWeapons;
    bool            m_IsPaused   = false;
    bool            m_FreeCursor = false;  // 2キー: 広報・スクショ用カーソル解放
    float           m_PlayTime   = 0.0f;  // ポーズ中は停止するゲーム内経過時間（秒）
    std::string     m_StartTime;           // プレイ開始日時（ログ用）

    // 現在プレイ中のステージデータ。GameContext::stageDB が所有するポインタ。
    // Endless モード時またはロード失敗時は nullptr になる。
    const StageData* m_CurrentStage = nullptr;
};
