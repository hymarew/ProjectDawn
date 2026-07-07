#pragma once
#include <vector>
#include "spiderEnemy.h"

class GameObject;

// =====================================================
// SpiderPool : SpiderEnemy のオブジェクトプール
//
// EnemyPool と同じ設計。蜘蛛は別種として独立したプールを持つ。
// Manager::g_SpiderPool として gameScene.cpp が Init/Uninit/Update を呼ぶ。
// =====================================================
class SpiderPool
{
public:
    SpiderPool()  {}
    ~SpiderPool() {}

    void Init(int maxCount);
    void Uninit();
    void Update(float dt);

    SpiderEnemy* SpawnSpider(const Vector3& pos, GameObject* target);

    int GetActiveCount() const;
    int GetMaxCount()    const { return m_MaxCount; }
    int GetKillCount()   const { return m_KillCount; }
    void ResetKillCount()      { m_KillCount = 0; }

    std::vector<SpiderEnemy*>& GetActiveSpiders() { return m_ActiveSpiders; }
    std::vector<SpiderEnemy>&  GetPool()           { return m_Pool; }

private:
    std::vector<SpiderEnemy>  m_Pool;
    std::vector<SpiderEnemy*> m_ActiveSpiders;
    int m_MaxCount  = 0;
    int m_KillCount = 0;
};
