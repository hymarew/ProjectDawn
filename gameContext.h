#pragma once
#include "gameMode.h"
#include "stageID.h"
#include "stageDatabase.h"

// =====================================================
// GameContext : シーンをまたいで共有するゲーム状態
//
// MenuScene / StageSelectScene で設定した値を
// GameScene / ResultScene へ受け渡す目的でシングルトンにしている。
// StageDatabase もここに置くことで、ResultScene での解放処理が
// StageSelectScene の表示に即時反映される。
// =====================================================
class GameContext
{
public:
    static GameContext& Instance();

    GameMode      currentMode  = GameMode::Story;   // MenuScene で設定される
    StageID       currentStage = StageID::Stage1;   // StageSelectScene で設定される
    StageDatabase stageDB;                          // 解放状態を含むステージ一覧

private:
    GameContext() = default;
};
