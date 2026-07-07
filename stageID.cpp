#include "stageID.h"
#include <cctype>

namespace
{
    const std::string PREFIX = "stage";
}

// ---------------------------------------------------------
// StageIDToKey : StageID(1) → "stage1"
// ---------------------------------------------------------
std::string StageIDToKey(StageID id)
{
    return PREFIX + std::to_string(id.number);
}

// ---------------------------------------------------------
// KeyToStageID : "stage1" → StageID(1)
//
// "stage" に続く部分がすべて数字のときだけ成功として扱う。
// これにより stage1 / stage2 / stage11 のように桁数が増えても
// 追記なしで対応できる。
// ---------------------------------------------------------
bool KeyToStageID(const std::string& key, StageID& outID)
{
    if (key.size() <= PREFIX.size()) return false;
    if (key.compare(0, PREFIX.size(), PREFIX) != 0) return false;

    for (size_t i = PREFIX.size(); i < key.size(); i++)
    {
        if (!std::isdigit(static_cast<unsigned char>(key[i])))
            return false;
    }

    outID = StageID(std::stoi(key.substr(PREFIX.size())));
    return true;
}
