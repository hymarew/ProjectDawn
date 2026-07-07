#pragma once
#include "component.h"
#include "vector3.h"

// =====================================================
// ColliderTag : コライダーの種別識別子
// =====================================================
enum class ColliderTag { Player, Enemy, Bullet, Item, Spawner, Obstacle, Box };

// =====================================================
// ColliderShape : 型ディスパッチ用（dynamic_cast 不要）
// =====================================================
enum class ColliderShape { Sphere, Box };

// =====================================================
// Collider : SphereCollider / BoxCollider の共通基底
//
// 将来の拡張例:
//   class CapsuleCollider : public Collider
//   class OBBCollider     : public Collider
//
// Trigger イベント（OnTriggerEnter 等）を追加する場合は
// このクラスに virtual callback を定義する。
// =====================================================
class Collider : public Component
{
public:
    Collider(GameObject* obj) : Component(obj) {}

    ColliderTag   GetTag()    const { return m_Tag; }
    ColliderShape GetShape()  const { return m_Shape; }
    Vector3       GetCenter() const { return m_Center; }

protected:
    Vector3       m_Center = {};
    ColliderTag   m_Tag    = ColliderTag::Player;
    ColliderShape m_Shape  = ColliderShape::Sphere;
};
