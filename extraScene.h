#pragma once
#include "scene.h"
#include <functional>
#include <vector>

// =====================================================
// ExtraScene : 高難易度コンテンツの選択画面
//
// ストーリークリア後に MainMenu の EXTRA から入る。
// 現在プレイできるのは ENDLESS のみで、BOSS RUSH / CHALLENGE / SURVIVAL は
// 将来の追加枠として Coming Soon 表示にしている。
//
// 【拡張方法】
//   MainMenuScene と同じデータ駆動方式。新モードを追加するときは
//   BuildItems() に1エントリ足し、対応する GameMode を増やすだけでよい。
//   モードごとの解放条件（例: BossRush は実績◯◯で解放）も
//   isLocked に UnlockManager の判定を書くだけで対応できる。
// =====================================================
class ExtraScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;

private:
    struct ExtraItem
    {
        const char*           label;       // 表示名
        const char*           description; // 選択中に表示する説明文
        std::function<bool()> isLocked;    // nullptr = 選択可能
        const char*           lockedHint;  // ロック中の表示（"Coming Soon" 等）
        std::function<void()> onDecide;    // 決定時の処理
    };

    void BuildItems();

    std::vector<ExtraItem> m_Items;
    int                    m_Selected = 0;
};
