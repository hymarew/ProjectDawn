#pragma once
#include <vector>

class EnemySpawner;

// =====================================================
// StageManager : ステージ全体のスポナー状態と統計を管理する
//
// WaveManager と連携して動作する:
//   - Wave開始時: WaveManager が ClearSpawners() し、新Spawnerを AddSpawner() で登録
//   - Wave終了時: WaveManager が AddDestroyedCount() で累積カウントを更新
//   - Stage終了時: Finalize(killCount) で ResultScene 用の最終統計を確定
// =====================================================
class StageManager
{
public:
    // ゲーム開始時に全状態をリセットする（GameScene::Init から呼ぶ）
    void Reset();

    // スポナーを登録する（Wave開始時に WaveManager から呼ぶ）
    void AddSpawner(EnemySpawner* spawner);

    // 現在Wave のスポナー参照をクリアする（次Wave開始前に呼ぶ）
    void ClearSpawners();

    // 撃破済みスポナー数を累積する（Wave終了時に WaveManager から呼ぶ）
    void AddDestroyedCount(int n);

    // ステージ終了前に最終統計を確定させる（GameScene::Uninit の直前に呼ぶ）
    void Finalize(int killCount, float playTime);

    // 現在Wave の全スポナーが破壊済みか
    bool IsCurrentWaveSpawnersClear() const;

    // 現在Wave で生存しているスポナーの数
    int GetAliveCount() const;

    // 現在Wave で破壊済みスポナーの数
    int GetDestroyedCount() const;

    // 現在Wave の登録スポナー総数
    int GetTotalCount() const { return (int)m_Spawners.size(); }

    // 登録済みスポナーへの読み取りアクセス（HUD描画など）
    const std::vector<EnemySpawner*>& GetSpawners() const { return m_Spawners; }

    // ---- リザルト用（Finalize 後に有効） ----
    int   GetResultDestroyedCount() const { return m_ResultDestroyedCount; }
    int   GetResultKillCount()      const { return m_ResultKillCount; }
    float GetResultPlayTime()       const { return m_ResultPlayTime; }

    // ゲームオーバーフラグ（プレイヤー死亡時に GameScene がセットする）
    void SetGameOver()    { m_IsGameOver = true; }
    bool IsGameOver() const { return m_IsGameOver; }

private:
    std::vector<EnemySpawner*> m_Spawners;

    // Wave をまたいで累積する破壊済みスポナー数
    int m_TotalDestroyedAcrossWaves = 0;

    // Finalize で確定させる最終統計
    int   m_ResultDestroyedCount = 0;
    int   m_ResultKillCount      = 0;
    float m_ResultPlayTime       = 0.0f;
    bool  m_IsGameOver           = false;
};

extern StageManager g_StageManager;
