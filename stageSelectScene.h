#pragma once
#include "scene.h"
#include <d3d11.h>
#include <unordered_map>

// =====================================================
// StageSelectScene : ステージ選択画面（Story モード専用）
//
// GameContext::stageDB からステージ一覧を取得して表示する。
// 解放済みステージのみ表示・選択可能。
// 決定時に GameContext::currentStage を設定してから GameScene へ遷移する。
// =====================================================
class StageSelectScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;

private:
    int m_Selected = 0;  // 解放済みステージ一覧内のカーソル位置

    // ステージごとのサムネイル画像（キー: StageID を int にキャストした値）
    std::unordered_map<int, ID3D11ShaderResourceView*> m_Thumbnails;
};
