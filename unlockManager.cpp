// ===================================================
// unlockManager.cpp
// コンテンツ解放状態の一元判定の実装
// ===================================================

#include "unlockManager.h"
#include "saveManager.h"

namespace
{
    // ストーリークリアの記録キー（save.json の unlocks セクション）
    // ※ 同セクションは武器所持("weapon101"等)と共用のため衝突しない名前にする
    constexpr const char* KEY_STORY_CLEAR = "storyClear";
}

namespace UnlockManager
{
    bool IsUnlocked(const std::string& key)
    {
        const auto& unlocks = g_SaveManager.GetData().unlocks;
        auto it = unlocks.find(key);
        return (it != unlocks.end()) && it->second;
    }

    void Unlock(const std::string& key)
    {
        g_SaveManager.GetData().unlocks[key] = true;
        g_SaveManager.Save(); // 解放は重要イベントなので即時保存する
    }

    bool IsStoryCleared()
    {
        return IsUnlocked(KEY_STORY_CLEAR);
    }

    void MarkStoryCleared()
    {
        // 二重保存を避ける（既にクリア済みなら何もしない）
        if (IsStoryCleared()) return;
        Unlock(KEY_STORY_CLEAR);
    }
}
