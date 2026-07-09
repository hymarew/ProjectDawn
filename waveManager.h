#pragma once
#include <vector>
#include "stageData.h"  // Init の引数 const StageData* と WaveConfig への変換に使用

class Player;

// =====================================================
// WaveConfig : WaveManager が内部で使う1Wave分の実行時設定
//
// stageData.h の WaveData（JSON 由来）とは別物。
// WaveData.spawnerCount から SpawnerCount を取り、
// SpawnRate / AnnounceDuration はゲーム側で付与する。
//
// EnemyCount > 0 のときは直接生成方式（スポナーを使わずWave開始時に
// SpawnPos 周辺へ EnemyCount 体を一斉生成する）を使う。
// =====================================================
struct WaveConfig
{
    int   SpawnerCount;      // このWaveで生成するスポナー数（スポナー方式）
    float SpawnRate;         // スポナー1体あたりの生成速度（体/秒）
    float AnnounceDuration;  // "WAVE X" 表示時間（秒）

    int     EnemyCount  = 0;     // 一斉生成する敵数（直接生成方式。0なら未使用）
    bool    StartActive = true;  // true: Chase（Active）から開始 / false: Idle（巡回）から開始
    Vector3 SpawnPos    = {};    // 固定生成座標
};

// =====================================================
// WaveManager : Wave 単位でステージ進行を管理するクラス
//
// 責務:
//   - std::vector<WaveConfig> で全 Wave を保持する
//   - Wave 開始時にスポナーを生成し StageManager に登録する
//   - Wave 終了条件（敵 0 体 && スポナー全破壊）を毎フレーム判定する
//   - 全 Wave 終了で IsAllWavesCleared() = true となる
//
// Wave 状態遷移:
//   Idle
//   → StartFirstWave() で Announcing
//   → アナウンス時間経過で Active（スポナー生成）
//   → IsCurrentWaveCleared() で WaitingNext
//   → 待機時間後に次 Wave の Announcing
//   → 繰り返し
//   → 最終 Wave クリアで m_AllWavesCleared = true
//
// 設計注意:
//   シーン遷移は GameScene 側が IsAllWavesCleared() を見て行う。
//   WaveManager は SceneManager を直接呼ばない。
// =====================================================
class WaveManager
{
public:
    // 初期化。stage が nullptr の場合はフォールバック値で Wave を構築する。
    void Init(Player* target, const StageData* stage);
    void Uninit();

    // 毎フレーム呼ぶ
    void Update(float dt);

    // 最初の Wave を開始する（GameScene::Init の末尾で呼ぶ）
    void StartFirstWave();

    // ---- 状態取得 ----

    // 全 Wave が終了したか（GameScene がシーン遷移のトリガーに使う）
    bool IsAllWavesCleared() const { return m_AllWavesCleared; }

    // 現在の Wave 番号（1-indexed で HUD 表示用）
    int GetCurrentWave() const { return m_CurrentWave + 1; }
    int GetTotalWaves()  const { return (int)m_Waves.size(); }

    // ---- アナウンス演出 ----
    bool        IsAnnouncing()        const;
    bool        IsWaitingNextWave()   const { return m_State == State::WaitingNext; }
    const char* GetAnnounceText()     const { return m_AnnounceText; }
    float       GetAnnounceAlpha()    const;  // 0.0〜1.0（フェードイン/アウト込み）
    float       GetWaitProgress()     const;  // 次Wave待機の進捗（0.0〜1.0）

private:
    enum class State { Idle, Announcing, Active, WaitingNext };

    void StartWave(int waveIndex);
    void SpawnWaveEntities(const WaveConfig& cfg);
    bool IsCurrentWaveCleared() const;

    std::vector<WaveConfig> m_Waves;
    int     m_CurrentWave    = -1;
    State   m_State          = State::Idle;
    Player* m_Target         = nullptr;

    float m_AnnounceTimer  = 0.0f;
    float m_NextWaveTimer  = 0.0f;
    bool  m_AllWavesCleared = false;

    char m_AnnounceText[32] = {};

    // 次 Wave 開始までの待機時間（秒）
    static constexpr float NEXT_WAVE_DELAY  = 5.0f;

    // スポナーのランダム配置距離（プレイヤー原点からの距離）
    static constexpr float SPAWNER_MIN_DIST = 15.0f;
    static constexpr float SPAWNER_MAX_DIST = 30.0f;

    // 直接生成方式: 固定座標周辺に敵をばらけさせるオフセット半径（同一座標への重なり防止）
    static constexpr float ENEMY_SPAWN_OFFSET_RANGE = 20.0f;
};
