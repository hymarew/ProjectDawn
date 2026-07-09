#pragma once
#include "enemy.h"
#include <vector>

class Scorpion;

// =====================================================
// EnemyPool : Scorpion を事前確保するオブジェクトプール
//
// 敵は Manager の GameObject リストには入れない。
// Update/Draw は gameScene.cpp から直接呼ばれる。
// =====================================================
class EnemyPool
{
public:
    void    Init(int maxCount);
    void    Uninit();
    void    Update(float dt);

    // startActive: true ならChase（Active）、false ならIdle（巡回）で生成する
    Enemy*  SpawnEnemy(const Vector3& pos, GameObject* target, bool startActive = true);

    int     GetActiveCount() const { return (int)Enemy::GetActiveList().size(); }
    int     GetKillCount()   const { return Enemy::GetTotalKills(); }
    void    ResetKillCount()       { Enemy::ResetKills(); }

    std::vector<Enemy*>& GetActiveEnemies() { return Enemy::GetActiveList(); }

private:
    std::vector<Scorpion*> m_Pool;
};

extern EnemyPool g_EnemyPool;
