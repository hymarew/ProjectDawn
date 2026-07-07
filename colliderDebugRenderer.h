#pragma once
#include "main.h"

class CollisionManager;
class EnemyPool;
class Camera;
class BoxCollider;

class ColliderDebugRenderer
{
public:
    void Draw(CollisionManager& colMgr, EnemyPool& enemyPool,
              Camera* cam, float thickness = 2.0f, int segments = 32);

private:
    void DrawSphere(ImDrawList* dl, const XMMATRIX& vp,
                    float cx, float cy, float cz, float radius,
                    ImU32 color, float thickness, int segments);

    void DrawBox(ImDrawList* dl, const XMMATRIX& vp,
                 const BoxCollider* box, ImU32 color, float thickness);
};

extern ColliderDebugRenderer g_ColliderDebugRenderer;
