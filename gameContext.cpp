#include "gameContext.h"
#include "worldItemPool.h"
#include "debugPrintf.h"

GameContext& GameContext::Instance()
{
    static GameContext s_Instance;
    return s_Instance;
}

// ---------------------------------------------------------
// コンストラクタ : 武器収集システムのデータを一括ロードする
//
// ロード順: マスタ（武器 → アイテム → ドロップテーブル）→ 所持品復元。
// バリデーションで参照切れ・未知の attackType を起動時に検出し、
// データ作成ミスを実行前に出力ウィンドウで気づけるようにする。
// ---------------------------------------------------------
GameContext::GameContext()
    : weaponFactory(&weaponDB)
    , itemFactory(&itemDB, &g_WorldItemPool)
{
    weaponDB.Load();
    itemDB.Load();
    dropDB.Load();

    // ---- ロード時バリデーション（失敗しても起動は続行し、警告のみ出す） ----
    if (!itemDB.Validate(weaponDB))
        DebugPrintf("[GameContext] ItemDatabase validation failed (missing weapon refs)\n");
    if (!dropDB.Validate(itemDB))
        DebugPrintf("[GameContext] DropTableDatabase validation failed (missing item refs)\n");
    if (!weaponFactory.ValidateDatabase())
        DebugPrintf("[GameContext] WeaponFactory validation failed (unknown attackType)\n");

    // 所持品をセーブから復元する（初回起動なら初期武器を付与する）
    inventory.Init(&weaponDB);

    // 装備をセーブから復元する（Inventory と照合するため必ず後に呼ぶ）
    equipLoadout.Init(&inventory);
}
