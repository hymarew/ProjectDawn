#pragma once
#include "scene.h"
#include <d3d11.h>

// =====================================================
// TitleScene : タイトル画面（起動の入口）
//   - Start / Exit の2項目のみ。Start で MainMenu（拠点）へ遷移する
//   - 実績・設定は拠点（MainMenuScene）側の項目へ移管した。
//     タイトルの責務は「ゲームを開始するかどうか」だけにする
//   - タイトルはテキストではなくロゴ画像（titleLogoDawn.png）で表示する
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

    // タイトルロゴ（読み込み失敗時は nullptr のままテキスト表示にフォールバックする）
    ID3D11ShaderResourceView* m_LogoSRV    = nullptr;
    float                     m_LogoAspect = 2.5f; // 幅÷高さ（読み込み時に実サイズで上書き）

    static constexpr int ITEM_COUNT = 2; // Start / Exit
};
