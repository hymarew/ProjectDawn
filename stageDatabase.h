#pragma once
#include <vector>
#include "stageData.h"

// =====================================================
// StageDatabase : ステージ一覧と解放状態を保持するクラス
//
// Data/Stages/*.json を StageLoader で読み込んで m_Stages を構築する。
// 解放状態は SaveManager から復元し、UnlockStage() 呼び出し時に
// SaveManager を経由して save.json へ即時書き込む。
// =====================================================
class StageDatabase
{
public:
    // コンストラクタで JSON ロードと SaveManager からの解放状態復元を行う。
    // g_SaveManager.Load() より後に呼ばれる必要がある。
    StageDatabase();

    const std::vector<StageData>& GetStages() const;

    // 見つからない場合は nullptr を返す
    const StageData* GetStage(StageID id) const;

    // 指定ステージを解放し、SaveManager を通じて save.json へ保存する。
    // 既に解放済みでも呼び出しは安全。
    void UnlockStage(StageID id);

    bool IsUnlocked(StageID id) const;

private:
    std::vector<StageData> m_Stages;

    // Data/Stages/ 以下の JSON を順に読み込んで m_Stages を構築する。
    // 1ファイルでも読み込み失敗したら false を返す。
    bool LoadStages();
};

// StageID を SaveManager のキー文字列へ変換する。
// 例: StageID::Stage1 → "stage1"
std::string StageIDToKey(StageID id);
