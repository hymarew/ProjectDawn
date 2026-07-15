#include "itemDatabase.h"
#include "weaponDatabase.h"
#include "jsonParse.h"
#include "debugPrintf.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <windows.h>   // FindFirstFile/FindNextFile（StageDatabase と同方式）

namespace
{
    constexpr const char* ITEMS_DIR = "Data/Items";

    // JSON 1ファイル分を ItemData へ変換する。
    // 必須キー（id / name / type）が欠けていれば false を返す。
    bool LoadItemFile(const std::string& path, ItemData& out)
    {
        std::ifstream f(path);
        if (!f.is_open()) return false;

        std::ostringstream ss;
        ss << f.rdbuf();
        const std::string src = ss.str();

        using namespace JsonParse;

        // ---- 必須キー ----
        int idNum = 0;
        if (!ParseInt(src, "id", idNum) || idNum <= 0) return false;
        out.id = ItemID(idNum);

        if (!ParseString(src, "name", out.name)) return false;

        std::string typeStr;
        if (!ParseString(src, "type", typeStr)) return false;
        if (!ItemTypeFromString(typeStr, out.type))
        {
            DebugPrintf("[ItemDatabase] unknown type '%s' : %s\n",
                        typeStr.c_str(), path.c_str());
            return false;
        }

        // ---- 任意キー ----
        std::string rarityStr;
        if (ParseString(src, "rarity", rarityStr))
        {
            if (!RarityFromString(rarityStr, out.rarity))
                DebugPrintf("[ItemDatabase] unknown rarity '%s' : %s\n",
                            rarityStr.c_str(), path.c_str());
        }

        ParseString(src, "model", out.modelPath);
        ParseString(src, "icon",  out.iconPath);

        // ---- type 別キー ----
        int weaponIDNum = 0;
        if (ParseInt(src, "weaponID", weaponIDNum))
            out.weaponID = WeaponID(weaponIDNum);

        ParseFloat(src, "healAmount", out.healAmount);

        // 武器アイテムなのに weaponID がないのはデータ作成ミス
        if (out.type == ItemType::Weapon && !out.weaponID.IsValid())
        {
            DebugPrintf("[ItemDatabase] weapon item without weaponID : %s\n", path.c_str());
            return false;
        }

        return true;
    }
}

// ---------------------------------------------------------
// Load : Data/Items/*.json を走査して構築する
// ---------------------------------------------------------
bool ItemDatabase::Load()
{
    m_Items.clear();
    m_Sorted.clear();

    WIN32_FIND_DATAA findData;
    std::string      pattern = std::string(ITEMS_DIR) + "\\*.json";
    HANDLE           hFind   = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DebugPrintf("[ItemDatabase] no item json found in %s\n", ITEMS_DIR);
        return false;
    }

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        std::string path = std::string(ITEMS_DIR) + "\\" + findData.cFileName;

        ItemData data;
        if (!LoadItemFile(path, data))
        {
            DebugPrintf("[ItemDatabase] failed to load : %s\n", path.c_str());
            continue;
        }

        if (m_Items.count(data.id.number) > 0)
            DebugPrintf("[ItemDatabase] duplicate ItemID %d : %s\n",
                        data.id.number, path.c_str());

        m_Items[data.id.number] = data;
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    m_Sorted.reserve(m_Items.size());
    for (const auto& kv : m_Items)
        m_Sorted.push_back(&kv.second);
    std::sort(m_Sorted.begin(), m_Sorted.end(),
        [](const ItemData* a, const ItemData* b) { return a->id < b->id; });

    DebugPrintf("[ItemDatabase] loaded %d items\n", (int)m_Items.size());
    return !m_Items.empty();
}

// ---------------------------------------------------------
// Validate : 武器アイテム → WeaponDatabase の参照切れ検出
// ---------------------------------------------------------
bool ItemDatabase::Validate(const WeaponDatabase& weaponDB) const
{
    bool ok = true;
    for (const auto& kv : m_Items)
    {
        const ItemData& item = kv.second;
        if (item.type != ItemType::Weapon) continue;

        if (weaponDB.Find(item.weaponID) == nullptr)
        {
            DebugPrintf("[ItemDatabase] item %d '%s' references missing WeaponID %d\n",
                        item.id.number, item.name.c_str(), item.weaponID.number);
            ok = false;
        }
    }
    return ok;
}

const ItemData* ItemDatabase::Find(ItemID id) const
{
    auto it = m_Items.find(id.number);
    return (it != m_Items.end()) ? &it->second : nullptr;
}

const ItemData* ItemDatabase::FindByWeaponID(WeaponID weaponID) const
{
    for (const auto& kv : m_Items)
        if (kv.second.type == ItemType::Weapon && kv.second.weaponID == weaponID)
            return &kv.second;
    return nullptr;
}
