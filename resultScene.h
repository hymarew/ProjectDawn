#pragma once
#include "scene.h"

// =====================================================
// ResultScene : リザルト画面
//   - STAGE CLEAR / GAME OVER を表示
//   - Retry（同ステージ再挑戦）または Stage Select へ戻る
// =====================================================
class ResultScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;

private:
    int m_Selected = 0;  // 0: Retry, 1: Stage Select
};
