#include "sphereCollider.h"
#include "gameObject.h"

void SphereCollider::Setup(float radius, ColliderTag tag)
{
    m_Radius = radius;
    m_Tag    = tag;
    m_Shape  = ColliderShape::Sphere;
}

void SphereCollider::Update(float dt)
{
    if (m_GameObject)
        m_Center = m_GameObject->GetPosition();
}

bool SphereCollider::Overlaps(const SphereCollider& other) const
{
    float r    = m_Radius + other.m_Radius;
    float dx   = m_Center.x - other.m_Center.x;
    float dy   = m_Center.y - other.m_Center.y;
    float dz   = m_Center.z - other.m_Center.z;
    return (dx * dx + dy * dy + dz * dz) < (r * r);
}
