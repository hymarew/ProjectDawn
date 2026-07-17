#include "main.h"
#include "colliderDebugRenderer.h"
#include "collisionManager.h"
#include "collider.h"
#include "sphereCollider.h"
#include "boxCollider.h"
#include "enemyPool.h"
#include "enemy.h"
#include "renderer.h"
#include "camera.h"
#include <cmath>
#include <vector>

ColliderDebugRenderer g_ColliderDebugRenderer;

static bool Project(const XMMATRIX& vp,
                    float wx, float wy, float wz,
                    float& sx, float& sy)
{
    XMVECTOR clip = XMVector4Transform(XMVectorSet(wx, wy, wz, 1.0f), vp);
    float w = XMVectorGetW(clip);
    if (w <= 0.0f) return false;

    sx = ( XMVectorGetX(clip) / w * 0.5f + 0.5f) * SCREEN_WIDTH;
    sy = (-XMVectorGetY(clip) / w * 0.5f + 0.5f) * SCREEN_HEIGHT;
    return true;
}

void ColliderDebugRenderer::DrawSphere(ImDrawList* dl, const XMMATRIX& vp,
                                       float cx, float cy, float cz, float r,
                                       ImU32 color, float thickness, int segs)
{
    for (int circle = 0; circle < 3; ++circle)
    {
        std::vector<ImVec2> pts;
        pts.reserve(segs + 1);

        for (int i = 0; i <= segs; ++i)
        {
            float a = (float)i / segs * XM_2PI;
            float px, py, pz;

            if (circle == 0)
            {
                px = cx + r * cosf(a);
                py = cy;
                pz = cz + r * sinf(a);
            }
            else if (circle == 1)
            {
                px = cx + r * cosf(a);
                py = cy + r * sinf(a);
                pz = cz;
            }
            else
            {
                px = cx;
                py = cy + r * cosf(a);
                pz = cz + r * sinf(a);
            }

            float sx, sy;
            if (Project(vp, px, py, pz, sx, sy))
                pts.push_back({ sx, sy });
            else if (!pts.empty())
            {
                if (pts.size() >= 2)
                    dl->AddPolyline(pts.data(), (int)pts.size(), color, 0, thickness);
                pts.clear();
            }
        }

        if (pts.size() >= 2)
            dl->AddPolyline(pts.data(), (int)pts.size(), color, 0, thickness);
    }
}

void ColliderDebugRenderer::DrawBox(ImDrawList* dl, const XMMATRIX& vp,
                                    const BoxCollider* box, ImU32 color, float thickness)
{
    Vector3 mn = box->GetMin();
    Vector3 mx = box->GetMax();

    float wx[8], wy[8], wz[8];
    for (int i = 0; i < 8; ++i)
    {
        wx[i] = (i & 1) ? mx.x : mn.x;
        wy[i] = (i & 2) ? mx.y : mn.y;
        wz[i] = (i & 4) ? mx.z : mn.z;
    }

    float sx[8], sy[8];
    bool  visible[8];
    for (int i = 0; i < 8; ++i)
        visible[i] = Project(vp, wx[i], wy[i], wz[i], sx[i], sy[i]);

    static const int edges[12][2] = {
        {0,1},{2,3},{4,5},{6,7},
        {0,2},{1,3},{4,6},{5,7},
        {0,4},{1,5},{2,6},{3,7}
    };

    for (auto& e : edges)
    {
        int a = e[0], b = e[1];
        if (visible[a] && visible[b])
            dl->AddLine({ sx[a], sy[a] }, { sx[b], sy[b] }, color, thickness);
    }
}

void ColliderDebugRenderer::Draw(CollisionManager& colMgr, EnemyPool& enemyPool,
                                 Camera* cam, float thickness, int segments)
{
    if (!cam) return;

    XMMATRIX    vp    = cam->GetViewMatrix() * cam->GetProjectionMatrix();
    ImDrawList* dl    = ImGui::GetBackgroundDrawList();
    ImU32 colorGreen  = IM_COL32(0,   255, 0,   200);
    ImU32 colorYellow = IM_COL32(255, 255, 0,   200);

    for (auto* col : colMgr.GetColliders())
    {
        Vector3 c = col->GetCenter();

        if (col->GetShape() == ColliderShape::Sphere)
        {
            auto* sph = static_cast<SphereCollider*>(col);
            DrawSphere(dl, vp, c.x, c.y, c.z, sph->GetRadius(),
                       colorGreen, thickness, segments);
        }
        else
        {
            DrawBox(dl, vp, static_cast<BoxCollider*>(col), colorYellow, thickness);
        }
    }

    for (auto* e : enemyPool.GetActiveEnemies())
    {
        SphereCollider* col = e->GetComponent<SphereCollider>();
        if (!col) continue;
        Vector3 c = col->GetCenter();
        DrawSphere(dl, vp, c.x, c.y, c.z, col->GetRadius(),
                   colorGreen, thickness, segments);
    }

    // ---- 弾の被弾判定球（HitSphere / マルチスフィア）----
    // 移動用コライダー（緑）と区別できるようシアンで描く。
    // モデルに重ねて表示し、球の位置・半径の調整に使う
    ImU32 colorCyan = IM_COL32(0, 220, 255, 200);
    for (auto* e : enemyPool.GetActiveEnemies())
    {
        int count = 0;
        const HitSphere* spheres = e->GetHitSpheres(count);
        const float   yaw = e->GetRotation().y;
        const Vector3 pos = e->GetPosition();

        for (int i = 0; i < count; i++)
        {
            Vector3 c = pos + spheres[i].LocalOffset.RotatedAroundY(yaw);
            DrawSphere(dl, vp, c.x, c.y, c.z, spheres[i].Radius,
                       colorCyan, thickness, segments);
        }
    }
}
