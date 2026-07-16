#pragma once
#include "gameMode.h"
#include "stageID.h"
#include "stageDatabase.h"
#include "weaponDatabase.h"
#include "itemDatabase.h"
#include "dropTableDatabase.h"
#include "weaponFactory.h"
#include "itemFactory.h"
#include "inventory.h"
#include "equipLoadout.h"

// =====================================================
// GameContext : シーンをまたいで共有するゲーム状態
//
// MenuScene / StageSelectScene で設定した値を
// GameScene / ResultScene へ受け渡す目的でシングルトンにしている。
// StageDatabase もここに置くことで、ResultScene での解放処理が
// StageSelectScene の表示に即時反映される。
//
// 武器収集システムのデータベース群・Factory・Inventory もここに置く。
// コンストラクタで JSON ロード → 参照バリデーション → 所持品復元を行う
// （初回の Instance() 呼び出しが g_SaveManager.Load() より後である前提。
//   StageDatabase と同じ制約）。
// =====================================================
class GameContext
{
public:
    static GameContext& Instance();

    GameMode      currentMode  = GameMode::Story;   // MenuScene で設定される
    StageID       currentStage = StageID(1);        // StageSelectScene で設定される
    StageDatabase stageDB;                          // 解放状態を含むステージ一覧

    // ---- 武器収集・アイテムドロップシステム ----
    WeaponDatabase    weaponDB;       // 武器マスタ（Data/Weapons）
    ItemDatabase      itemDB;         // アイテムマスタ（Data/Items）
    DropTableDatabase dropDB;         // ドロップテーブル（Data/DropTables）
    WeaponFactory     weaponFactory;  // WeaponID → Weapon 実体
    ItemFactory       itemFactory;    // ItemID → WorldItem（g_WorldItemPool 使用）
    Inventory         inventory;      // 所持武器（セーブと連携）
    EquipLoadout      equipLoadout;   // 装備の永続データ（WeaponSelectScene が編集）

private:
    GameContext();
};
