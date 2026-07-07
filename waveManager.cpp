#include "waveManager.h"
#include "main.h"
#include "manager.h"
#include "stageManager.h"
#include "enemySpawner.h"
#include "enemyPool.h"
#include "bulletPool.h"
#include "player.h"
#include "GameConfig.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

// =====================================================
// Init : StageData の waves から WaveConfig を構築する
//
// stage が nullptr の場合（Endless モードやロード失敗時）は
// フォールバック値でハードコードの Wave を生成する。
// =====================================================
void WaveManager::Init(Player* target, const StageData* stage)
{
    m_Target          = target;
    m_CurrentWave     = -1;
    m_State           = State::Idle;
    m_AllWavesCleared = false;
    m_AnnounceTimer   = 0.0f;
    m_NextWaveTimer   = 0.0f;
    m_AnnounceText[0] = '\0';
    m_Waves.clear();  // 再 Init 時に前回の Wave 設定が残らないよう明示的にクリアする

    if (stage && !stage->waves.empty())
    {
        // JSON の WaveData.spawnerCount を WaveConfig に変換する。
        // SpawnRate / AnnounceDuration は JSON に持たせないためここで付与する。
        for (const auto& wd : stage->waves)
        {
            WaveConfig cfg;
            cfg.SpawnerCount     = wd.spawnerCount;
            cfg.SpawnRate        = GameConfig::Spawner::DEFAULT_SPAWN_RATE;
            cfg.AnnounceDuration = 2.0f;
            m_Waves.push_back(cfg);
        }
    }
    else
    {
        // stage が nullptr（Endless モード）またはwavesが空（JSON 不正）の場合のフォールバック。
        // { SpawnerCount, SpawnRate(体/秒), AnnounceDuration(秒) }
        m_Waves = {
            { 1, GameConfig::Spawner::DEFAULT_SPAWN_RATE,        2.0f },
            { 2, GameConfig::Spawner::DEFAULT_SPAWN_RATE,        2.0f },
            { 2, GameConfig::Spawner::DEFAULT_SPAWN_RATE * 2.0f, 2.0f },
        };
    }
}

void WaveManager::Uninit()
{
    m_Target = nullptr;
}

// =====================================================
// StartFirstWave : GameScene::Init から呼ぶ
// =====================================================
void WaveManager::StartFirstWave()
{
    StartWave(0);
}

// =====================================================
// StartWave : 指定Waveの開始処理
// =====================================================
void WaveManager::StartWave(int waveIndex)
{
    if (waveIndex < 0 || waveIndex >= (int)m_Waves.size()) return;

    m_CurrentWave = waveIndex;

    snprintf(m_AnnounceText, sizeof(m_AnnounceText),
        "WAVE %d", m_CurrentWave + 1);

    m_AnnounceTimer = m_Waves[m_CurrentWave].AnnounceDuration;
    m_State         = State::Announcing;

    // 前Waveのスポナー参照をクリア（新Waveで新しいスポナーを登録するため）
    g_StageManager.ClearSpawners();
}

// =====================================================
// SpawnWaveEntities : スポナーをランダム配置で生成・登録する
// =====================================================
void WaveManager::SpawnWaveEntities(const WaveConfig& cfg)
{
    for (int i = 0; i < cfg.SpawnerCount; i++)
    {
        // 均等な角度でスポナーを配置する（重なりを防ぐ）
        float angle = (float)i / (float)cfg.SpawnerCount * 6.28318f  // 2π
                    + ((float)rand() / RAND_MAX) * 0.5f;              // 少しランダムにずらす

        float dist = SPAWNER_MIN_DIST
                   + ((float)rand() / RAND_MAX) * (SPAWNER_MAX_DIST - SPAWNER_MIN_DIST);

        Vector3 pos = {
            cosf(angle) * dist,
            0.0f,
            sinf(angle) * dist
        };

        EnemySpawner* spawner = Manager::AddGameObject<EnemySpawner>();
        spawner->SetPosition(pos);
        spawner->SetPoolAndTarget(&g_EnemyPool, m_Target);
        spawner->SetSpawnRate(cfg.SpawnRate);

        g_BulletPool.RegisterSpawner(spawner);
        g_StageManager.AddSpawner(spawner);
    }
}

// =====================================================
// IsCurrentWaveCleared : 現Waveのクリア条件判定
// 敵0体 && 現Wave全スポナー破壊済み
// =====================================================
bool WaveManager::IsCurrentWaveCleared() const
{
    if (m_State != State::Active) return false;
    if (g_EnemyPool.GetActiveCount() > 0) return false;
    return g_StageManager.IsCurrentWaveSpawnersClear();
}

// =====================================================
// Update : 毎フレームの状態遷移
// =====================================================
void WaveManager::Update(float dt)
{
    switch (m_State)
    {
    // アナウンス表示中 → 時間が経過したらスポナーを生成して Active へ
    case State::Announcing:
        m_AnnounceTimer -= dt;
        if (m_AnnounceTimer <= 0.0f)
        {
            m_State = State::Active;
            SpawnWaveEntities(m_Waves[m_CurrentWave]);
        }
        break;

    // 戦闘中 → クリア条件を満たしたら次の処理へ
    case State::Active:
        if (IsCurrentWaveCleared())
        {
            // 今Waveで破壊したスポナー数を累積
            g_StageManager.AddDestroyedCount(g_StageManager.GetDestroyedCount());

            // 最終Waveか？
            if (m_CurrentWave + 1 >= (int)m_Waves.size())
            {
                m_AllWavesCleared = true;
            }
            else
            {
                m_State         = State::WaitingNext;
                m_NextWaveTimer = NEXT_WAVE_DELAY;
            }
        }
        break;

    // 次Wave待機 → 待機時間が経過したら次Waveへ
    case State::WaitingNext:
        m_NextWaveTimer -= dt;
        if (m_NextWaveTimer <= 0.0f)
            StartWave(m_CurrentWave + 1);
        break;

    default:
        break;
    }
}

// =====================================================
// アナウンス演出用のゲッター
// =====================================================
bool WaveManager::IsAnnouncing() const
{
    return m_State == State::Announcing;
}

float WaveManager::GetWaitProgress() const
{
    if (m_State != State::WaitingNext) return 0.0f;
    return 1.0f - (m_NextWaveTimer / NEXT_WAVE_DELAY);
}

float WaveManager::GetAnnounceAlpha() const
{
    if (!IsAnnouncing() || m_CurrentWave < 0) return 0.0f;

    const float dur = m_Waves[m_CurrentWave].AnnounceDuration;
    if (dur <= 0.0f) return 0.0f;

    float t = m_AnnounceTimer / dur;  // 1.0（開始直後）→ 0.0（終了直前）

    // 最初の 30%: フェードイン（0 → 1）
    if (t > 0.7f) return (1.0f - t) / 0.3f;

    // 最後の 30%: フェードアウト（1 → 0）
    if (t < 0.3f) return t / 0.3f;

    // 中間 40%: 全表示
    return 1.0f;
}
