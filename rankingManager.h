#pragma once
#include <vector>
#include <string>

// =====================================================
// RankEntry : ランキング 1 件分のデータ
// =====================================================
struct RankEntry
{
    float playTime;       // クリアタイム（秒）
    int   killCount;      // キル数
    int   destroyedCount; // 撃破スポナー数
};

// =====================================================
// RankingManager : STAGE CLEAR タイムランキング管理
//
// - クリア時間が短いほど上位
// - GAME OVER はランキング対象外
// - 上位 MAX_ENTRIES 件をファイルに永続化する
//
// 使い方（ResultScene 側）:
//   g_RankingManager.Load();
//   bool entered = g_RankingManager.TryRegister(entry);
//   if (entered) g_RankingManager.Save();
// =====================================================
class RankingManager
{
public:
    static constexpr int MAX_ENTRIES = 5;

    // ファイルからランキングを読み込む（存在しない場合は空のまま）
    void Load();

    // ファイルにランキングを書き込む
    void Save() const;

    // エントリーを追加してランキングを更新する。
    // ランキング圏内（クリア時間が既存エントリーより短い、または件数が MAX_ENTRIES 未満）
    // ならば登録して true を返す。それ以外は false。
    bool TryRegister(const RankEntry& entry);

    const std::vector<RankEntry>& GetEntries() const { return m_Entries; }

    // 直前に TryRegister で追加されたエントリーのインデックス（ハイライト用）
    // 登録されなかった場合は -1
    int GetNewEntryIndex() const { return m_NewIndex; }

private:
    std::vector<RankEntry> m_Entries;
    int                    m_NewIndex = -1;

    static const char* SAVE_PATH;
};

extern RankingManager g_RankingManager;
