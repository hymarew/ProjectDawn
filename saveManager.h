#pragma once
#include "saveData.h"

// =====================================================
// SaveManager : save.json の読み書きを担当するクラス
//
// StageDatabase と連携し、解放状態を永続化する（Step4-2）。
// =====================================================
class SaveManager
{
public:
    // save.json を読み込む。
    // ファイルが存在しない場合はデフォルトデータを生成して Save() を呼ぶ。
    // 戻り値: 読み込み成功 or 新規生成 = true、書き込み失敗 = false
    bool Load();

    // 現在の m_Data を save.json に書き出す。
    // 戻り値: 書き込み成功 = true
    bool Save();

    SaveData& GetData() { return m_Data; }
    const SaveData& GetData() const { return m_Data; }

    // ステージ解放状態の取得・設定。StageDatabase から呼ばれる。
    // stageId は "stage1" / "stage2" / "stage3" などのキー文字列。
    bool IsStageUnlocked(const std::string& stageId) const;
    void SetStageUnlocked(const std::string& stageId, bool unlocked);

    // オプション設定の取得・設定。キーが存在しない場合は defaultValue を返す。
    // OptionsMenu / SoundManager / InputManager から呼ばれる。
    float GetSettingFloat(const std::string& key, float defaultValue) const;
    void  SetSettingFloat(const std::string& key, float value);

    // 累計統計の取得・加算。AchievementManager から呼ばれる。
    int  GetStat(const std::string& key) const;
    void AddStat(const std::string& key, int amount);

private:
    SaveData m_Data;

    // save.json のパス。Data/ フォルダ以下に置く
    static constexpr const char* SAVE_PATH = "Data/save.json";

    // m_Data をデフォルト値で初期化する（新規セーブ時）
    void InitDefault();
};

extern SaveManager g_SaveManager;
