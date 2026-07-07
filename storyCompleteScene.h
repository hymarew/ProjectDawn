#pragma once
#include "scene.h"

// =====================================================
// StoryCompleteScene : ストーリー全クリア画面
//
// Stage3 クリア時に ResultScene から遷移してくる。
// 任意キー / クリックで StageSelectScene へ戻る。
// =====================================================
class StoryCompleteScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;
};
