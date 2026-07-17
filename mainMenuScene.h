#pragma once
#include "scene.h"
#include "optionsMenu.h"
#include "achievementScreen.h"
#include <functional>
#include <vector>

// =====================================================
// MainMenuScene : ゲーム全体のハブ画面（EDFシリーズの拠点メニュー相当）
//
// ここから 出撃 / 武器変更 / EXTRA / 実績 / 設定 / タイトルへ戻る の
// 各機能へアクセスする。ゲームの導線は必ずここを経由して戻ってくる。
//
// 【拡張方法】
//   メニュー項目はデータ駆動（m_Items の配列）で管理しており、
//   ショップ・図鑑・武器強化などを追加する場合は BuildItems() に
//   1エントリ足すだけでよい（カーソル処理・描画・ロック表示は共通）。
//
// 【解放条件】
//   各項目は isLocked 判定（UnlockManager 経由）を持てる。
//   ロック中は鍵マークと解放条件のヒントを表示し、決定を受け付けない。
// =====================================================
class MainMenuScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;

private:
    // メニュー1項目分の定義
    struct MenuItem
    {
        const char*           label;       // 表示名
        const char*           description; // 選択中に下部へ表示する説明文
        std::function<bool()> isLocked;    // nullptr = 常に選択可能
        const char*           lockedHint;  // ロック中に表示する解放条件（"Clear Story" 等）
        std::function<void()> onDecide;    // 決定時の処理（シーン遷移 or オーバーレイ）
    };

    // メニュー項目の一覧を構築する（項目追加はここに足すだけ）
    void BuildItems();

    std::vector<MenuItem> m_Items;
    int                   m_Selected = 0;

    OptionsMenu       m_Options;      // 設定オーバーレイ（TitleSceneから移管）
    AchievementScreen m_Achievements; // 実績一覧オーバーレイ（TitleSceneから移管）
};
