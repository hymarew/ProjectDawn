#include "stageDatabase.h"
#include "stageLoader.h"
#include "saveManager.h"
#include <cstdio>  // printf（エラーログ用）

// ---------------------------------------------------------
// StageIDToKey : StageID を SaveManager のキー文字列へ変換する
// ---------------------------------------------------------
std::string StageIDToKey(StageID id)
{
    switch (id)
    {
    case StageID::Stage1: return "stage1";
    case StageID::Stage2: return "stage2";
    case StageID::Stage3: return "stage3";
    default:              return "";  // 未知の ID は空文字列。SaveManager 側でキー不在として安全に処理される。
    }
}

// ---------------------------------------------------------
// LoadStages : JSON ファイルから m_Stages を構築する
//
// 読み込み順序がステージ選択画面の表示順になるため、
// stage1 → stage2 → stage3 の順で固定している。
// ---------------------------------------------------------
bool StageDatabase::LoadStages()
{
    // paths の順序 = StageSelectScene の表示順になる。
    // ステージを追加する場合はここに追記する。
    static const char* paths[] = {
        "Data/Stages/stage1.json",
        "Data/Stages/stage2.json",
        "Data/Stages/stage3.json",
    };

    StageLoader loader;
    for (const char* path : paths)
    {
        StageData stage;
        if (!loader.Load(path, stage))
        {
            // 失敗したファイル名を出力して即座に中断する。
            // m_Stages が中途半端な状態にならないよう clear してから返す。
            printf("Failed Load Stage : %s\n", path);
            m_Stages.clear();
            return false;
        }
        // loader.Load が unlocked を false で返すため、
        // 解放状態の上書きはコンストラクタ末尾の SaveManager 参照に任せる
        m_Stages.push_back(stage);
    }
    return true;
}

// ---------------------------------------------------------
// コンストラクタ : JSON ロード後に SaveManager から解放状態を復元する
// ---------------------------------------------------------
StageDatabase::StageDatabase()
{
    if (!LoadStages())
    {
        // JSON ロード失敗時は m_Stages が空のまま続行する。
        // StageSelectScene は count == 0 のとき何も表示しないため安全に動作する。
        return;
    }

    // JSON には unlocked を持たせない設計のため、
    // 解放状態は SaveManager からここで上書きする。
    // Manager::Init() で g_SaveManager.Load() が先に呼ばれていることが前提。
    for (auto& s : m_Stages)
    {
        s.unlocked = g_SaveManager.IsStageUnlocked(StageIDToKey(s.id));
    }
}

const std::vector<StageData>& StageDatabase::GetStages() const
{
    return m_Stages;
}

const StageData* StageDatabase::GetStage(StageID id) const
{
    for (const auto& s : m_Stages)
    {
        if (s.id == id)
            return &s;
    }
    return nullptr;  // 存在しない ID が渡された場合。呼び出し側で null チェックが必要。
}

void StageDatabase::UnlockStage(StageID id)
{
    for (auto& s : m_Stages)
    {
        if (s.id == id)
        {
            s.unlocked = true;

            // メモリ上の状態を更新したら SaveManager にも書き込んで即座に保存する。
            // Save() をここで呼ぶことで、アプリ終了前にクラッシュしても
            // クリア済み状態が失われないようにする。
            std::string key = StageIDToKey(id);
            g_SaveManager.SetStageUnlocked(key, true);
            g_SaveManager.Save();

            return;  // ID は一意なので見つかり次第終了
        }
    }
    // 存在しない ID が渡された場合は何もしない（呼び出し側が誤っていても安全）
}

bool StageDatabase::IsUnlocked(StageID id) const
{
    const StageData* s = GetStage(id);
    return s && s->unlocked;
}
