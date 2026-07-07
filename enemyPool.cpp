#include "enemyPool.h"
#include "scorpion.h"

EnemyPool g_EnemyPool;

// =====================================================
// Init : 最大数分の Scorpion を事前確保する
// =====================================================
void EnemyPool::Init(int maxCount)
{
    m_Pool.reserve(maxCount);
    for (int i = 0; i < maxCount; i++)
    {
        Scorpion* s = new Scorpion();
        s->Init();
        m_Pool.push_back(s);
    }
}

// =====================================================
// Uninit : 全インスタンスを解放する
// =====================================================
void EnemyPool::Uninit()
{
    Enemy::GetActiveList().clear();
    Enemy::ResetKills();

    for (Scorpion* s : m_Pool)
    {
        s->Uninit();
        delete s;
    }
    m_Pool.clear();
}

// =====================================================
// SpawnEnemy : 非アクティブスロットを探してスポーンする
// =====================================================
Enemy* EnemyPool::SpawnEnemy(const Vector3& pos, GameObject* target)
{
    for (Scorpion* s : m_Pool)
    {
        if (!s->IsAlive())
        {
            s->Spawn(pos, target);
            return s;
        }
    }
    return nullptr; // プール満杯
}

// =====================================================
// Update : リストのコピーを取ってから更新する
//          Update 中に Kill() が呼ばれてもリスト破壊が起きない
// =====================================================
void EnemyPool::Update(float dt)
{
    auto list = Enemy::GetActiveList(); // コピー
    for (Enemy* e : list)
        e->Update(dt);
}
