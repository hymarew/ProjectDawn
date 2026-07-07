#include "spiderPool.h"
#include <algorithm>

SpiderPool g_SpiderPool;

void SpiderPool::Init(int maxCount)
{
    m_MaxCount = maxCount;
    m_Pool.resize(maxCount);

    for (auto& spider : m_Pool)
    {
        spider.Init();
        spider.m_IsActive = false;
    }
}

void SpiderPool::Uninit()
{
    for (auto& spider : m_Pool)
        spider.Uninit();
    m_Pool.clear();
}

void SpiderPool::Update(float dt)
{
    for (auto& spider : m_Pool)
    {
        if (spider.m_IsActive)
            spider.Update(dt);
    }

    // 死亡した蜘蛛をキルカウントに加算してアクティブリストから除外する
    for (SpiderEnemy* s : m_ActiveSpiders)
        if (!s->m_IsActive) m_KillCount++;

    m_ActiveSpiders.erase(
        std::remove_if(m_ActiveSpiders.begin(), m_ActiveSpiders.end(),
            [](SpiderEnemy* s) { return !s->m_IsActive; }),
        m_ActiveSpiders.end()
    );
}

SpiderEnemy* SpiderPool::SpawnSpider(const Vector3& pos, GameObject* target)
{
    for (auto& spider : m_Pool)
    {
        if (!spider.m_IsActive)
        {
            spider.Spawn(pos, target);
            m_ActiveSpiders.push_back(&spider);
            return &spider;
        }
    }
    return nullptr;  // プール上限に達している場合は何も生成しない
}

int SpiderPool::GetActiveCount() const
{
    int count = 0;
    for (const auto& s : m_Pool)
        if (s.m_IsActive) count++;
    return count;
}
