#pragma once
#include "vector3.h"
#include <string>
#include <map>

// =====================================================
// PlayerLog : 1プレイ分の統計データ
//
// 各システムが直接フィールドを書き込む。
// ゲーム終了時に Save() で JSON ファイルとして出力する。
// =====================================================
struct PlayerLog
{
    std::string startTime;   // プレイ開始日時
    std::string endTime;     // プレイ終了日時

    int   maxWave  = 0;      // 到達した最大Wave番号（1-indexed）
    float playTime = 0.0f;   // クリア時間（秒）

    int   totalKills       = 0;    // 総キル数
    float totalDamageTaken = 0.0f; // 総被ダメージ量

    int   shotsFired = 0;    // 発射した弾の総数（ペレット単位）
    int   shotsHit   = 0;    // 敵に命中した弾の数

    // 武器ごとの発射回数（トリガー操作回数ベース）。使用率算出に使う
    std::map<std::string, int> weaponShotsFired;

    // 敵タイプごとの撃破数
    std::map<std::string, int> enemyKills;

    // 敵タイプごとの被弾回数（被弾回数ベース）
    std::map<std::string, int> damageSources;

    Vector3     deathPosition = { 0.0f, 0.0f, 0.0f }; // 死亡座標
    std::string deathCause;                             // 死亡原因（敵タイプ名 or "Clear"）

    // ゲーム開始時に呼ぶ。全フィールドを初期値に戻す
    void Reset();

    // derived fields（accuracy, weaponUsage %）を計算して JSON に書き出す
    void Save() const;
};

extern PlayerLog g_PlayerLog;
