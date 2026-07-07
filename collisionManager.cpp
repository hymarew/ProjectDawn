#include "main.h"
#include "collisionManager.h"
#include "collider.h"
#include "sphereCollider.h"
#include "boxCollider.h"
#include "enemyPool.h"
#include "enemy.h"
#include "player.h"
#include "GameConfig.h"
#include <algorithm>
#include <cmath>

void CollisionManager::Register(Collider* col)
{
    m_Colliders.push_back(col);
}

void CollisionManager::Unregister(Collider* col)
{
    m_Colliders.erase(
        std::remove(m_Colliders.begin(), m_Colliders.end(), col),
        m_Colliders.end());
}

bool CollisionManager::CheckOverlap(Collider* a, Collider* b) const
{
    bool aIsSphere = (a->GetShape() == ColliderShape::Sphere);
    bool bIsSphere = (b->GetShape() == ColliderShape::Sphere);

    if (aIsSphere && bIsSphere)
        return static_cast<SphereCollider*>(a)->Overlaps(*static_cast<SphereCollider*>(b));

    if (!aIsSphere && !bIsSphere)
        return static_cast<BoxCollider*>(a)->Overlaps(*static_cast<BoxCollider*>(b));

    if (!aIsSphere)
        return static_cast<BoxCollider*>(a)->Overlaps(*static_cast<SphereCollider*>(b));
    else
        return static_cast<BoxCollider*>(b)->Overlaps(*static_cast<SphereCollider*>(a));
}

void CollisionManager::Update()
{
    for (int i = 0; i < (int)m_Colliders.size(); i++)
        for (int j = i + 1; j < (int)m_Colliders.size(); j++)
            if (CheckOverlap(m_Colliders[i], m_Colliders[j]))
                Resolve(m_Colliders[i], m_Colliders[j]);
}

static void PushOutFromSphere(GameObject* mover,
                               const Vector3& fixedCenter, float fixedR, float moverR)
{
    Vector3 moverPos = mover->GetPosition();
    Vector3 dir      = moverPos - fixedCenter;
    dir.y = 0.0f;

    float len = dir.Length();
    if (len < 0.001f) { dir = Vector3(1.0f, 0.0f, 0.0f); len = 1.0f; }

    float penetration = (fixedR + moverR) - len;
    if (penetration > 0.0f)
    {
        dir = dir * (1.0f / len);
        moverPos += dir * penetration;
        mover->SetPosition(moverPos);
    }
}

static void PushOutFromBox(GameObject* mover, const BoxCollider* box, float moverR)
{
    Vector3 moverPos = mover->GetPosition();
    Vector3 boxMin   = box->GetMin();
    Vector3 boxMax   = box->GetMax();

    float cx = (std::max)(boxMin.x, (std::min)(moverPos.x, boxMax.x));
    float cz = (std::max)(boxMin.z, (std::min)(moverPos.z, boxMax.z));

    float dx = moverPos.x - cx;
    float dz = moverPos.z - cz;
    float len = sqrtf(dx * dx + dz * dz);

    if (len < 0.001f)
    {
        float dLeft  = moverPos.x - boxMin.x;
        float dRight = boxMax.x   - moverPos.x;
        float dFront = moverPos.z - boxMin.z;
        float dBack  = boxMax.z   - moverPos.z;

        float minD = dLeft; Vector3 pushDir(-1, 0, 0);
        if (dRight < minD) { minD = dRight; pushDir = Vector3( 1, 0,  0); }
        if (dFront < minD) { minD = dFront; pushDir = Vector3( 0, 0, -1); }
        if (dBack  < minD) { minD = dBack;  pushDir = Vector3( 0, 0,  1); }

        moverPos.x += pushDir.x * (minD + moverR);
        moverPos.z += pushDir.z * (minD + moverR);
        mover->SetPosition(moverPos);
        return;
    }

    float pen = moverR - len;
    if (pen > 0.0f)
    {
        moverPos.x += (dx / len) * pen;
        moverPos.z += (dz / len) * pen;
        mover->SetPosition(moverPos);
    }
}

static bool PushOutFromBox3D(GameObject* mover, const BoxCollider* box, float moverR)
{
    Vector3 pos    = mover->GetPosition();
    Vector3 boxMin = box->GetMin();
    Vector3 boxMax = box->GetMax();

    float penXN = (pos.x + moverR) - boxMin.x;
    float penXP = boxMax.x - (pos.x - moverR);
    float penYN = (pos.y + moverR) - boxMin.y;
    float penYP = boxMax.y - (pos.y - moverR);
    float penZN = (pos.z + moverR) - boxMin.z;
    float penZP = boxMax.z - (pos.z - moverR);

    if (penXN <= 0 || penXP <= 0 || penYN <= 0 || penYP <= 0 || penZN <= 0 || penZP <= 0)
        return false;

    struct Face { float pen; float nx, ny, nz; };
    Face faces[6] = {
        { penXN, -1,  0,  0 },
        { penXP,  1,  0,  0 },
        { penYN,  0, -1,  0 },
        { penYP,  0,  1,  0 },
        { penZN,  0,  0, -1 },
        { penZP,  0,  0,  1 },
    };

    int best = 0;
    for (int i = 1; i < 6; ++i)
        if (faces[i].pen < faces[best].pen) best = i;

    pos.x += faces[best].nx * faces[best].pen;
    pos.y += faces[best].ny * faces[best].pen;
    pos.z += faces[best].nz * faces[best].pen;
    mover->SetPosition(pos);

    return (faces[best].ny > 0.0f);
}

void CollisionManager::Resolve(Collider* a, Collider* b)
{
    auto isObstacle = [](ColliderTag t) {
        return t == ColliderTag::Obstacle || t == ColliderTag::Box;
    };

    Collider* obstacleSide = nullptr;
    Collider* playerSide   = nullptr;

    if (isObstacle(a->GetTag()) && b->GetTag() == ColliderTag::Player)
        { obstacleSide = a; playerSide = b; }
    else if (a->GetTag() == ColliderTag::Player && isObstacle(b->GetTag()))
        { obstacleSide = b; playerSide = a; }

    if (!obstacleSide || !playerSide) return;

    float playerR = (playerSide->GetShape() == ColliderShape::Sphere)
                  ? static_cast<SphereCollider*>(playerSide)->GetRadius()
                  : GameConfig::Collision::PLAYER_RADIUS;

    if (obstacleSide->GetShape() == ColliderShape::Sphere)
    {
        auto* sph = static_cast<SphereCollider*>(obstacleSide);
        PushOutFromSphere(playerSide->GetOwner(),
                          sph->GetCenter(), sph->GetRadius(), playerR);
    }
    else
    {
        bool landed = PushOutFromBox3D(playerSide->GetOwner(),
                                       static_cast<BoxCollider*>(obstacleSide), playerR);
        if (landed)
        {
            Player* p = static_cast<Player*>(playerSide->GetOwner());
            p->ZeroFallVelocity();
        }
    }
}

// -------------------------------------------------------
// CheckEnemyVsPlayer : エネミー vs プレイヤーの接触判定
// -------------------------------------------------------
void CollisionManager::CheckEnemyVsPlayer(EnemyPool& enemyPool, Player* player)
{
    if (!player || !player->IsAlive()) return;

    SphereCollider* playerCol = player->GetComponent<SphereCollider>();
    if (!playerCol) return;

    Vector3 pCenter = playerCol->GetCenter();
    const float r2  = (GameConfig::Collision::SCORPION_RADIUS + playerCol->GetRadius())
                    * (GameConfig::Collision::SCORPION_RADIUS + playerCol->GetRadius());

    for (Enemy* e : enemyPool.GetActiveEnemies())
    {
        Vector3 pos = e->GetPosition();
        float dx = pos.x - pCenter.x;
        float dy = pos.y - pCenter.y;
        float dz = pos.z - pCenter.z;

        if (dx * dx + dy * dy + dz * dz < r2)
        {
            player->TakeDamage(GameConfig::Scorpion::MELEE_DAMAGE, e->GetTypeName());
            break;
        }
    }
}

// -------------------------------------------------------
// CheckObstacleVsEnemies : 障害物 vs エネミーの押し返し
// -------------------------------------------------------
void CollisionManager::CheckObstacleVsEnemies(EnemyPool& enemyPool)
{
    for (Collider* col : m_Colliders)
    {
        if (col->GetTag() != ColliderTag::Obstacle &&
            col->GetTag() != ColliderTag::Box) continue;

        if (col->GetShape() == ColliderShape::Sphere)
        {
            auto*         sph     = static_cast<SphereCollider*>(col);
            const Vector3 center  = sph->GetCenter();
            const float   obsR    = sph->GetRadius();
            const float   totalR2 = (obsR + GameConfig::Collision::SCORPION_RADIUS)
                                  * (obsR + GameConfig::Collision::SCORPION_RADIUS);

            for (Enemy* e : enemyPool.GetActiveEnemies())
            {
                Vector3 diff = e->GetPosition() - center;
                diff.y = 0.0f;
                if (diff.LengthSq() < totalR2)
                    PushOutFromSphere(e, center, obsR, GameConfig::Collision::SCORPION_RADIUS);
            }
        }
        else
        {
            auto* box = static_cast<BoxCollider*>(col);
            for (Enemy* e : enemyPool.GetActiveEnemies())
            {
                Vector3 hs    = box->GetHalfSize();
                float   maxHS = hs.x > hs.y ? (hs.x > hs.z ? hs.x : hs.z) : (hs.y > hs.z ? hs.y : hs.z);
                float   approxR = maxHS + GameConfig::Collision::SCORPION_RADIUS;
                Vector3 diff  = e->GetPosition() - box->GetCenter();
                diff.y = 0.0f;
                if (diff.LengthSq() > approxR * approxR * 4.0f) continue;

                SphereCollider* sc = e->GetComponent<SphereCollider>();
                if (sc && box->Overlaps(*sc))
                    PushOutFromBox(e, box, GameConfig::Collision::SCORPION_RADIUS);
            }
        }
    }
}
