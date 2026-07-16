#include "weaponDatabase.h"
#include "jsonParse.h"
#include "debugPrintf.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <windows.h>   // FindFirstFile/FindNextFile（StageDatabase と同方式）

namespace
{
    constexpr const char* WEAPONS_DIR = "Data/Weapons";

    // JSON 1ファイル分を WeaponData へ変換する。
    // 必須キー（id / name / category）が欠けていれば false を返す。
    bool LoadWeaponFile(const std::string& path, WeaponData& out)
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
        out.id = WeaponID(idNum);

        if (!ParseString(src, "name", out.name)) return false;

        std::string categoryStr;
        if (!ParseString(src, "category", categoryStr)) return false;
        if (!WeaponCategoryFromString(categoryStr, out.category))
        {
            DebugPrintf("[WeaponDatabase] unknown category '%s' : %s\n",
                        categoryStr.c_str(), path.c_str());
            return false;
        }

        // ---- 任意キー（欠けていればデフォルト値のまま） ----
        std::string rarityStr;
        if (ParseString(src, "rarity", rarityStr))
        {
            if (!RarityFromString(rarityStr, out.rarity))
                DebugPrintf("[WeaponDatabase] unknown rarity '%s' : %s\n",
                            rarityStr.c_str(), path.c_str());
        }

        ParseInt  (src, "magazineSize",   out.magazineSize);
        ParseFloat(src, "fireRate",       out.fireRate);
        ParseFloat(src, "reloadTime",     out.reloadTime);
        ParseFloat(src, "zoomRatio",      out.zoomRatio);
        ParseBool (src, "singleShot",     out.singleShot);

        ParseFloat(src, "damage",         out.damage);
        ParseFloat(src, "bulletSpeed",    out.bulletSpeed);
        ParseFloat(src, "bulletLifeTime", out.bulletLifeTime);
        ParseFloat(src, "spreadAngle",    out.spreadAngle);
        ParseFloat(src, "splashRadius",   out.splashRadius);
        ParseFloat(src, "knockbackPower", out.knockbackPower);

        ParseString(src, "attackType", out.attackType);
        ParseString(src, "model",      out.modelPath);
        ParseString(src, "icon",       out.iconPath);

        return true;
    }
}

// ---------------------------------------------------------
// Load : Data/Weapons/*.json を走査して構築する
// ---------------------------------------------------------
bool WeaponDatabase::Load()
{
    m_Weapons.clear();
    m_Sorted.clear();

    WIN32_FIND_DATAA findData;
    std::string      pattern = std::string(WEAPONS_DIR) + "\\*.json";
    HANDLE           hFind   = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DebugPrintf("[WeaponDatabase] no weapon json found in %s\n", WEAPONS_DIR);
        return false;
    }

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        std::string path = std::string(WEAPONS_DIR) + "\\" + findData.cFileName;

        WeaponData data;
        if (!LoadWeaponFile(path, data))
        {
            DebugPrintf("[WeaponDatabase] failed to load : %s\n", path.c_str());
            continue;
        }

        // ID 重複はデータ作成ミスの可能性が高いので必ず警告する（後勝ちで上書き）
        if (m_Weapons.count(data.id.number) > 0)
            DebugPrintf("[WeaponDatabase] duplicate WeaponID %d : %s\n",
                        data.id.number, path.c_str());

        m_Weapons[data.id.number] = data;
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    // 一覧表示用に ID 昇順のビューを作る
    m_Sorted.reserve(m_Weapons.size());
    for (const auto& kv : m_Weapons)
        m_Sorted.push_back(&kv.second);
    std::sort(m_Sorted.begin(), m_Sorted.end(),
        [](const WeaponData* a, const WeaponData* b) { return a->id < b->id; });

    DebugPrintf("[WeaponDatabase] loaded %d weapons\n", (int)m_Weapons.size());
    return !m_Weapons.empty();
}

const WeaponData* WeaponDatabase::Find(WeaponID id) const
{
    auto it = m_Weapons.find(id.number);
    return (it != m_Weapons.end()) ? &it->second : nullptr;
}
