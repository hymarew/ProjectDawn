#include "stageDatabase.h"
#include "stageLoader.h"
#include "saveManager.h"
#include <cstdio>      // printf（エラーログ用）
#include <algorithm>   // std::sort
#include <windows.h>   // FindFirstFile/FindNextFile（<filesystem>はC++17のため使わない）

namespace
{
    constexpr const char* STAGES_DIR = "Data/Stages";
}

// ---------------------------------------------------------
// LoadStages : Data/Stages/*.json を走査して m_Stages を構築する
//
// stage1, stage2, stage11 のように番号が飛んでいても、
// フォルダ内の *.json をそのまま列挙するだけで対応できる。
// 表示順はステージ番号の昇順に揃える。
// ---------------------------------------------------------
bool StageDatabase::LoadStages()
{
    m_Stages.clear();

    WIN32_FIND_DATAA findData;
    std::string      pattern = std::string(STAGES_DIR) + "\\*.json";
    HANDLE           hFind   = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    StageLoader loader;
    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        std::string path = std::string(STAGES_DIR) + "\\" + findData.cFileName;

        StageData stage;
        if (!loader.Load(path, stage))
        {
            // 壊れたファイルはログを出してスキップし、他のステージは読み込みを続行する
            printf("Failed Load Stage : %s\n", path.c_str());
            continue;
        }
        // loader.Load が unlocked を false で返すため、
        // 解放状態の上書きはコンストラクタ末尾の SaveManager 参照に任せる
        m_Stages.push_back(stage);
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    std::sort(m_Stages.begin(), m_Stages.end(),
        [](const StageData& a, const StageData& b) { return a.id.number < b.id.number; });

    return !m_Stages.empty();
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

const StageData* StageDatabase::GetNextStage(StageID id) const
{
    // m_Stages は LoadStages() でステージ番号の昇順にソート済みのため、
    // id を見つけた次の要素がそのまま「次のステージ」になる。
    for (size_t i = 0; i < m_Stages.size(); i++)
    {
        if (m_Stages[i].id == id)
        {
            if (i + 1 < m_Stages.size()) return &m_Stages[i + 1];
            return nullptr;  // 最後のステージだった
        }
    }
    return nullptr;  // id が見つからない
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
