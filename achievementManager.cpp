#include "main.h"
#include "achievementManager.h"
#include "saveManager.h"
#include "renderer.h"

#include <cstring>

AchievementManager g_AchievementManager;

// -------------------------------------------------------
// 実績定義テーブル
// 実績を増やすときはここに1行足す（キーは save.json に載るため変更しない）
// -------------------------------------------------------
static const std::vector<AchievementDef> s_Definitions =
{
    { "firstClear",    "First Victory",   "Clear any stage for the first time"     },
    { "storyComplete", "World Savior",    "Clear the final stage"                  },
    { "noDamageClear", "Untouchable",     "Clear a stage without taking damage"    },
    { "kills100",      "Hunter",          "Defeat 100 enemies in total"            },
    { "kills1000",     "Exterminator",    "Defeat 1000 enemies in total"           },
    { "legendary",     "Golden Arsenal",  "Bring back a Legendary weapon"          },
    { "collector",     "Full Collection", "Collect every weapon"                   },
};

const std::vector<AchievementDef>& AchievementManager::GetDefinitions()
{
    return s_Definitions;
}

// -------------------------------------------------------
// FindDef : キーから定義を探す（テーブル外のキーは nullptr）
// -------------------------------------------------------
static const AchievementDef* FindDef(const char* key)
{
    for (const auto& def : s_Definitions)
        if (std::strcmp(def.key, key) == 0) return &def;
    return nullptr;
}

// -------------------------------------------------------
// Unlock : 初回解放なら保存してトーストを積む
// -------------------------------------------------------
bool AchievementManager::Unlock(const char* key)
{
    const AchievementDef* def = FindDef(key);
    if (!def) return false;  // 定義に無いキーは無視する（タイプミス対策）

    if (IsUnlocked(key)) return false;

    g_SaveManager.GetData().achievements[key] = true;
    g_SaveManager.Save();

    m_Toasts.push_back({ def, 0.0f });
    return true;
}

bool AchievementManager::IsUnlocked(const char* key) const
{
    const auto& map = g_SaveManager.GetData().achievements;
    auto it = map.find(key);
    return (it != map.end()) && it->second;
}

int AchievementManager::GetUnlockedCount() const
{
    int count = 0;
    for (const auto& def : s_Definitions)
        if (IsUnlocked(def.key)) ++count;
    return count;
}

// -------------------------------------------------------
// CheckKillMilestones : 累計キル数の実績をまとめて判定する
// -------------------------------------------------------
void AchievementManager::CheckKillMilestones(int totalKills)
{
    if (totalKills >= 100)  Unlock("kills100");
    if (totalKills >= 1000) Unlock("kills1000");
}

// -------------------------------------------------------
// Update : 先頭のトーストだけ時間を進め、表示終了で取り除く
// （複数同時解放でも1件ずつ順番に表示される）
// -------------------------------------------------------
void AchievementManager::Update(float dt)
{
    if (m_Toasts.empty()) return;

    m_Toasts.front().timer += dt;
    if (m_Toasts.front().timer >= TOAST_DURATION)
        m_Toasts.erase(m_Toasts.begin());
}

// -------------------------------------------------------
// DrawToasts : 画面上部中央に通知バナーを描画する
// -------------------------------------------------------
void AchievementManager::DrawToasts()
{
    if (m_Toasts.empty()) return;

    const Toast& toast = m_Toasts.front();
    ImDrawList*  dl    = ImGui::GetForegroundDrawList();
    ImFont*      font  = ImGui::GetFont();

    // フェードイン(0.3s)・フェードアウト(0.5s)
    float alpha = 1.0f;
    if      (toast.timer < 0.3f)                  alpha = toast.timer / 0.3f;
    else if (toast.timer > TOAST_DURATION - 0.5f) alpha = (TOAST_DURATION - toast.timer) / 0.5f;
    const int a = (int)(alpha * 255.0f);

    const float headerSz = 15.0f;
    const float nameSz   = 24.0f;
    const char* header   = "ACHIEVEMENT UNLOCKED";

    // バナーサイズは実績名の幅に合わせる
    ImVec2 ns = font->CalcTextSizeA(nameSz, FLT_MAX, 0.0f, toast.def->name);
    const float bannerW = ns.x + 120.0f;
    const float bannerH = 62.0f;
    const float bx      = (SCREEN_WIDTH - bannerW) * 0.5f;
    const float by      = 70.0f;  // スポナーHPゲージと被らない高さ

    dl->AddRectFilled(
        ImVec2(bx, by), ImVec2(bx + bannerW, by + bannerH),
        IM_COL32(20, 18, 5, (int)(220 * alpha)), 8.0f);
    dl->AddRect(
        ImVec2(bx, by), ImVec2(bx + bannerW, by + bannerH),
        IM_COL32(255, 200, 60, (int)(220 * alpha)), 8.0f, 0, 1.5f);

    // ヘッダー（金色の小文字）
    {
        ImVec2 hs = font->CalcTextSizeA(headerSz, FLT_MAX, 0.0f, header);
        dl->AddText(font, headerSz,
            ImVec2(bx + (bannerW - hs.x) * 0.5f, by + 8.0f),
            IM_COL32(255, 200, 60, a), header);
    }

    // 実績名（白の大きめ文字）
    dl->AddText(font, nameSz,
        ImVec2(bx + (bannerW - ns.x) * 0.5f, by + 28.0f),
        IM_COL32(255, 255, 255, a), toast.def->name);
}
