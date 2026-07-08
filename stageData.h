#pragma once
#include <string>
#include <vector>
#include "stageID.h"
#include "vector3.h"

// 1Wave 分の構成データ。StageLoader が JSON から生成する。
//
// スポナー方式（従来）: spawnerCount > 0 のときスポナーを設置し、
//                        時間経過で敵を少しずつ生成する（Endless モード等）。
// 直接生成方式（新方式）: enemyCount > 0 のとき、Wave開始時に spawnPos 周辺へ
//                          敵を一斉生成する（スポナーは使わない）。
//                          両方指定された場合は enemyCount を優先する。
struct WaveData
{
    int spawnerCount = 0;  // そのWaveで起動するスポナー数（スポナー方式）

    int     enemyCount  = 0;     // 一斉生成する敵数（直接生成方式。0なら未使用）
    bool    startActive = true;  // true: 出現直後からChase（Active）/ false: Idle（巡回）から開始
    Vector3 spawnPos    = {};    // 固定生成座標（ワールド座標、直接生成方式でのみ使用）
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
