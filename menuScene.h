#pragma once
#include "scene.h"

// =====================================================
// MenuScene : ゲームモード選択画面
//   STORY / ENDLESS / EXIT を縦並びで表示する。
//   選択後に GameContext::currentMode を設定してから
//   GameScene へ遷移する。
// =====================================================
class MenuScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;

private:
    int m_Selected = 0;  // 現在カーソルがある項目インデックス
};
