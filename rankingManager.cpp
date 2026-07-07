#include "rankingManager.h"
#include <fstream>
#include <algorithm>

RankingManager g_RankingManager;

const char* RankingManager::SAVE_PATH = "ranking.dat";

// -------------------------------------------------------
// Load : ファイルからランキングを読み込む
// -------------------------------------------------------
void RankingManager::Load()
{
    m_Entries.clear();
    m_NewIndex = -1;

    std::ifstream ifs(SAVE_PATH);
    if (!ifs.is_open()) return;

    RankEntry entry;
    while (ifs >> entry.playTime >> entry.killCount >> entry.destroyedCount)
    {
        m_Entries.push_back(entry);
        if ((int)m_Entries.size() >= MAX_ENTRIES) break;
    }
}

// -------------------------------------------------------
// Save : ファイルにランキングを書き込む
// -------------------------------------------------------
void RankingManager::Save() const
{
    std::ofstream ofs(SAVE_PATH);
    if (!ofs.is_open()) return;

    for (const RankEntry& e : m_Entries)
        ofs << e.playTime << " " << e.killCount << " " << e.destroyedCount << "\n";
}

// -------------------------------------------------------
// TryRegister : エントリーを追加してランキングを更新する
// -------------------------------------------------------
bool RankingManager::TryRegister(const RankEntry& entry)
{
    m_NewIndex = -1;

    // ランキングが満杯 かつ 最下位より遅い場合は対象外
    if ((int)m_Entries.size() >= MAX_ENTRIES &&
        entry.playTime >= m_Entries.back().playTime)
    {
        return false;
    }

    m_Entries.push_back(entry);

    // クリアタイムの昇順（短い順）でソート
    std::sort(m_Entries.begin(), m_Entries.end(),
        [](const RankEntry& a, const RankEntry& b)
        {
            return a.playTime < b.playTime;
        });

    // MAX_ENTRIES を超えた分を削除
    if ((int)m_Entries.size() > MAX_ENTRIES)
        m_Entries.resize(MAX_ENTRIES);

    // 追加されたエントリーのインデックスを特定（ソート後の位置）
    for (int i = 0; i < (int)m_Entries.size(); i++)
    {
        // playTime で一致するものを新エントリーとみなす
        // 同タイムが複数ある場合は最後のものを優先
        if (m_Entries[i].playTime == entry.playTime &&
            m_Entries[i].killCount == entry.killCount)
        {
            m_NewIndex = i;
        }
    }

    return true;
}
