#pragma once
#include <vector>

class Collider;
class EnemyPool;
class Player;

class CollisionManager
{
public:
    void Register(Collider* col);
    void Unregister(Collider* col);

    void Update();

    void CheckEnemyVsPlayer   (EnemyPool& enemyPool, Player* player);
    void CheckObstacleVsEnemies(EnemyPool& enemyPool);

    const std::vector<Collider*>& GetColliders() const { return m_Colliders; }

private:
    std::vector<Collider*> m_Colliders;

    bool CheckOverlap(Collider* a, Collider* b) const;
    void Resolve(Collider* a, Collider* b);
};
