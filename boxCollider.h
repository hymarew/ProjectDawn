#pragma once
#include "collider.h"

class SphereCollider;

// =====================================================
// BoxCollider : AABB（軸平行バウンディングボックス）
//
// m_Size はボックスの半径（各軸の半分のサイズ）を表す。
// 例）m_Size = {1, 1, 1} のとき、中心から ±1m の立方体。
// =====================================================
class BoxCollider : public Collider
{
public:
    BoxCollider(GameObject* obj) : Collider(obj) { m_Shape = ColliderShape::Box; }

    // ハーフサイズとタグを設定する（AddComponent 直後に呼ぶ）
    void Setup(const Vector3& halfSize, ColliderTag tag);

    // 毎フレーム GameObject の位置に中心座標を同期する
    void Update(float dt) override;

    Vector3 GetHalfSize() const { return m_HalfSize; }
    Vector3 GetMin()      const { return m_Center - m_HalfSize; }
    Vector3 GetMax()      const { return m_Center + m_HalfSize; }

    // AABB vs AABB
    bool Overlaps(const BoxCollider& other) const;

    // AABB vs Sphere
    bool Overlaps(const SphereCollider& sphere) const;

private:
    Vector3 m_HalfSize = { 1.0f, 1.0f, 1.0f };
};
