#pragma once

// 選択可能なステージの識別子。
// GameContext::currentStage に保持し StageSelectScene → GameScene へ受け渡す。
enum class StageID
{
    Stage1,
    Stage2,
    Stage3,
};
