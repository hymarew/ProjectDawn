#include "boxCollider.h"
#include "sphereCollider.h"
#include "gameObject.h"
#include <algorithm>
#include <cmath>

void BoxCollider::Setup(const Vector3& halfSize, ColliderTag tag)
{
    m_HalfSize = halfSize;
    m_Tag      = tag;
    m_Shape    = ColliderShape::Box;
}

void BoxCollider::Update(float dt)
{
    if (m_GameObject)
        m_Center = m_GameObject->GetPosition();
}

bool BoxCollider::Overlaps(const BoxCollider& other) const
{
    Vector3 aMin = GetMin();
    Vector3 aMax = GetMax();
    Vector3 bMin = other.GetMin();
    Vector3 bMax = other.GetMax();

    return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
           (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
           (aMin.z <= bMax.z && aMax.z >= bMin.z);
}

bool BoxCollider::Overlaps(const SphereCollider& sphere) const
{
    // 球の中心から AABB 上の最近傍点までの距離 < 半径 → 当たり
    Vector3 boxMin = GetMin();
    Vector3 boxMax = GetMax();
    Vector3 sc     = sphere.GetCenter();
    float   sr     = sphere.GetRadius();

    // (std::max) の括弧は Windows マクロ max との衝突を防ぐ
    float cx = (std::max)(boxMin.x, (std::min)(sc.x, boxMax.x));
    float cy = (std::max)(boxMin.y, (std::min)(sc.y, boxMax.y));
    float cz = (std::max)(boxMin.z, (std::min)(sc.z, boxMax.z));

    float dx = cx - sc.x;
    float dy = cy - sc.y;
    float dz = cz - sc.z;

    return (dx * dx + dy * dy + dz * dz) < (sr * sr);
}
