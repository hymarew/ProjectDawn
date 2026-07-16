#pragma once
#include <vector>

// =====================================================
// AchievementDef : 実績1件分の定義（静的マスタデータ）
//
// key は save.json の achievements セクションのキーになる。
// 実績を増やすときは achievementManager.cpp の定義テーブルに
// 1行足すだけでよい（一覧画面・解放処理は自動で対応する）。
// =====================================================
struct AchievementDef
{
    const char* key;   // セーブキー（例: "firstClear"）
    const char* name;  // 表示名（例: "First Victory"）
    const char* desc;  // 解放条件の説明文
};

// =====================================================
// AchievementManager : 実績の解放判定と通知トースト管理
//
// - 解放状態は SaveManager（save.json の achievements セクション）に永続化する
// - Unlock() は初回解放時のみ true を返し、トーストをキューに積む
// - トーストはシーンをまたいで残るため、GameScene 中の解放は
//   そのまま ResultScene でも表示され続ける
//
// 使い方:
//   解放:   g_AchievementManager.Unlock("firstClear");
//   毎フレーム: Update(dt) → DrawToasts()（GameScene / ResultScene）
//   一覧:   GetDefinitions() + IsUnlocked(key)（タイトルの実績画面）
// =====================================================
class AchievementManager
{
public:
    // 実績を解放する。初回解放なら save.json へ即時保存し、
    // 通知トーストを積んで true を返す。解放済みなら何もしない。
    bool Unlock(const char* key);

    bool IsUnlocked(const char* key) const;

    // 累計キル数 totalKills に応じたキル系実績をまとめて判定する。
    // GameScene から毎フレーム呼ばれる想定（解放済みはスキップされるため軽量）
    void CheckKillMilestones(int totalKills);

    // 通知トーストの時間経過（表示中のシーンの Update から呼ぶ）
    void Update(float dt);

    // 通知トーストの描画（画面上部中央。表示中のシーンの Draw から呼ぶ）
    void DrawToasts();

    // 全実績の定義一覧（タイトルの実績画面用）
    static const std::vector<AchievementDef>& GetDefinitions();

    // 解放済みの実績数（一覧画面の "3 / 7" 表示用）
    int GetUnlockedCount() const;

private:
    // 表示待ち・表示中のトースト
    struct Toast
    {
        const AchievementDef* def;
        float                 timer = 0.0f;  // 表示経過時間
    };

    std::vector<Toast> m_Toasts;

    static constexpr float TOAST_DURATION = 4.0f;  // 1件あたりの表示時間（秒）
};

extern AchievementManager g_AchievementManager;
