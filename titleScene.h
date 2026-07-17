#pragma once
#include "scene.h"

// =====================================================
// TitleScene : タイトル画面（起動の入口）
//   - Start / Exit の2項目のみ。Start で MainMenu（拠点）へ遷移する
//   - 実績・設定は拠点（MainMenuScene）側の項目へ移管した。
//     タイトルの責務は「ゲームを開始するかどうか」だけにする
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

    static constexpr int ITEM_COUNT = 2; // Start / Exit
};
