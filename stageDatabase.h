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

    // ステージ番号順に並んだ一覧の中で、id の次に位置するステージを返す。
    // id が最後のステージ、または存在しない場合は nullptr を返す。
    // 番号が連番でなくても（例: stage1, stage2, stage11）正しく次を判定できる。
    const StageData* GetNextStage(StageID id) const;

    // 指定ステージを解放し、SaveManager を通じて save.json へ保存する。
    // 既に解放済みでも呼び出しは安全。
    void UnlockStage(StageID id);

    bool IsUnlocked(StageID id) const;

private:
    std::vector<StageData> m_Stages;

    // Data/Stages/ 以下の *.json を走査して m_Stages を構築する。
    // 読み込みに失敗したファイルはログを出してスキップし、他のステージの
    // 読み込みは継続する。1件も読み込めなければ false を返す。
    bool LoadStages();
};
