#include "stageManager.h"
#include "enemySpawner.h"

StageManager g_StageManager;

void StageManager::Reset()
{
    m_Spawners.clear();
    m_TotalDestroyedAcrossWaves = 0;
    m_ResultDestroyedCount      = 0;
    m_ResultKillCount           = 0;
    m_ResultPlayTime            = 0.0f;
    m_IsGameOver                = false;
}

void StageManager::AddSpawner(EnemySpawner* spawner)
{
    m_Spawners.push_back(spawner);
}

void StageManager::ClearSpawners()
{
    m_Spawners.clear();
}

void StageManager::AddDestroyedCount(int n)
{
    m_TotalDestroyedAcrossWaves += n;
}

void StageManager::Finalize(int killCount, float playTime)
{
    m_ResultDestroyedCount = m_TotalDestroyedAcrossWaves;
    m_ResultKillCount      = killCount;
    m_ResultPlayTime       = playTime;
}

bool StageManager::IsCurrentWaveSpawnersClear() const
{
    if (m_Spawners.empty()) return false;
    for (const EnemySpawner* s : m_Spawners)
        if (!s->IsDestroyed()) return false;
    return true;
}

int StageManager::GetAliveCount() const
{
    int count = 0;
    for (const EnemySpawner* s : m_Spawners)
        if (!s->IsDestroyed()) count++;
    return count;
}

int StageManager::GetDestroyedCount() const
{
    int count = 0;
    for (const EnemySpawner* s : m_Spawners)
        if (s->IsDestroyed()) count++;
    return count;
}
