#pragma once
#include <string>
#include <vector>
#include "stageID.h"

// 1Wave 分の構成データ。StageLoader が JSON から生成する。
// 将来的に敵タイプや出現数など詳細情報を追加予定。
struct WaveData
{
    int spawnerCount = 0;  // そのWaveで起動するスポナー数
};

// 1ステージ分のメタ情報。StageDatabase が保持する。
// thumbnailPath は将来のサムネイル画像パス用（現段階はダミー表示）。
// unlocked は起動時に StageDatabase のコンストラクタで初期化され、
// クリア時に StageDatabase::UnlockStage() で更新される。
// waves は StageLoader が JSON から読み込む（StageDatabase の固定データとは別管理）。
struct StageData
{
    StageID     id;
    std::string name;
    std::string description;
    std::string thumbnailPath;
    bool        unlocked = false;       // 解放済みか（SaveManager と連携）
    std::vector<WaveData> waves;        // Wave 構成。StageDatabase / GameScene とは未接続（Step4-3A 時点）
};
