#pragma once
#include "scene.h"
#include "optionsMenu.h"
#include "achievementScreen.h"

// =====================================================
// TitleScene : タイトル画面
//   - Start / Achievements / Options の3項目メニュー
//   - Start で MenuScene（モード選択）へ遷移
//   - Achievements / Options はオーバーレイで開く
// =====================================================
class TitleScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;

private:
    int m_SelectedIndex = 0;

    OptionsMenu       m_Options;       // オプション設定オーバーレイ
    AchievementScreen m_Achievements;  // 実績一覧オーバーレイ

    static constexpr int ITEM_COUNT = 3; // Start / Achievements / Options
};
