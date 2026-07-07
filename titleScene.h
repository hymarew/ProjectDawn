#pragma once
#include "scene.h"

// =====================================================
// TitleScene : タイトル画面
//   - タイトルテキストを表示
//   - Enter キーで GameScene へ遷移
// =====================================================
class TitleScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;
};
