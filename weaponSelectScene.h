#pragma once
#include "scene.h"
#include <vector>
#include "weaponData.h"

// =====================================================
// WeaponSelectScene : 武器変更画面
//
// 遷移: MainMenu(拠点) → ここ → MainMenu へ戻る
//
// 責務は「装備武器を変更する」だけ。出撃の導線は持たない。
// 所持武器一覧（Inventory + WeaponDatabase）を表示し、
// Primary / Secondary の装備を EquipLoadout へ書き込む。
// 装備は即 save.json に保存されるため、シーンをまたぐ
// 受け渡しコードは不要（ゲーム開始時に WeaponEquip が読む）。
//
// 操作:
//   W/S or ↑↓ : カーソル移動
//   1 / 2      : Primary / Secondary に装備
//   Enter/ESC  : 拠点（MainMenu）へ戻る
// =====================================================
class WeaponSelectScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;

private:
    // 一覧1行分の表示データ（Inventory + Database + Loadout の合成結果）
    struct Entry
    {
        const WeaponData* data  = nullptr;
        bool              isNew = false;
        int               equippedSlot = -1;  // -1 = 未装備 / 0 = Primary / 1 = Secondary
    };

    // 所持武器一覧から表示用データを組み立てる
    std::vector<Entry> BuildEntries() const;

    int m_SelectedIndex = 0;

    static constexpr int   VISIBLE_ROWS = 10;     // 同時に表示する行数（超えたらスクロール）
    static constexpr float PANEL_W      = 860.0f;
};
