#include "dropTableDatabase.h"
#include "itemDatabase.h"
#include "jsonParse.h"
#include "debugPrintf.h"
#include <fstream>
#include <sstream>
#include <windows.h>   // FindFirstFile/FindNextFile（StageDatabase と同方式）

namespace
{
    constexpr const char* DROPTABLES_DIR = "Data/DropTables";

    // JSON 1ファイル分を DropTableData へ変換する。
    // 必須キー（enemyType / entries）が欠けていれば false を返す。
    bool LoadDropTableFile(const std::string& path, DropTableData& out)
    {
        std::ifstream f(path);
        if (!f.is_open()) return false;

        std::ostringstream ss;
        ss << f.rdbuf();
        const std::string src = ss.str();

        using namespace JsonParse;

        if (!ParseString(src, "enemyType", out.enemyTypeName)) return false;

        ParseString(src, "strategy",  out.strategyType);
        ParseFloat (src, "dropRate",  out.dropRate);
        ParseInt   (src, "rollCount", out.rollCount);
        if (out.rollCount < 1) out.rollCount = 1;

        // entries 配列: [ { "itemID": 1101, "weight": 5 }, ... ]
        out.entries.clear();
        ForEachArrayObject(src, "entries", [&out](const std::string& objSrc)
        {
            DropEntry entry;
            int idNum = 0;
            if (!ParseInt(objSrc, "itemID", idNum) || idNum <= 0) return;  // 不正エントリはスキップ
            entry.itemID = ItemID(idNum);

            ParseInt(objSrc, "weight", entry.weight);
            if (entry.weight < 1) entry.weight = 1;

            out.entries.push_back(entry);
        });

        return !out.entries.empty();
    }
}

// ---------------------------------------------------------
// Load : Data/DropTables/*.json を走査して構築する
// ---------------------------------------------------------
bool DropTableDatabase::Load()
{
    m_Tables.clear();

    WIN32_FIND_DATAA findData;
    std::string      pattern = std::string(DROPTABLES_DIR) + "\\*.json";
    HANDLE           hFind   = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DebugPrintf("[DropTableDatabase] no drop table json found in %s\n", DROPTABLES_DIR);
        return false;
    }

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        std::string path = std::string(DROPTABLES_DIR) + "\\" + findData.cFileName;

        DropTableData data;
        if (!LoadDropTableFile(path, data))
        {
            DebugPrintf("[DropTableDatabase] failed to load : %s\n", path.c_str());
            continue;
        }

        if (m_Tables.count(data.enemyTypeName) > 0)
            DebugPrintf("[DropTableDatabase] duplicate enemyType '%s' : %s\n",
                        data.enemyTypeName.c_str(), path.c_str());

        m_Tables[data.enemyTypeName] = data;
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    DebugPrintf("[DropTableDatabase] loaded %d tables\n", (int)m_Tables.size());
    return !m_Tables.empty();
}

// ---------------------------------------------------------
// Validate : entries → ItemDatabase の参照切れ検出
// ---------------------------------------------------------
bool DropTableDatabase::Validate(const ItemDatabase& itemDB) const
{
    bool ok = true;
    for (const auto& kv : m_Tables)
    {
        for (const DropEntry& entry : kv.second.entries)
        {
            if (itemDB.Find(entry.itemID) == nullptr)
            {
                DebugPrintf("[DropTableDatabase] table '%s' references missing ItemID %d\n",
                            kv.first.c_str(), entry.itemID.number);
                ok = false;
            }
        }
    }
    return ok;
}

const DropTableData* DropTableDatabase::Find(const std::string& enemyTypeName) const
{
    auto it = m_Tables.find(enemyTypeName);
    return (it != m_Tables.end()) ? &it->second : nullptr;
}
