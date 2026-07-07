#pragma once
#include "collider.h"

// ColliderTag と ColliderShape は collider.h で定義済み

class SphereCollider : public Collider
{
public:
    SphereCollider(GameObject* obj) : Collider(obj) {}

    // 半径とタグを設定する（AddComponent 直後に呼ぶ）
    void Setup(float radius, ColliderTag tag);

    // 毎フレーム GameObject の位置に中心座標を同期する
    void Update(float dt) override;

    float GetRadius() const { return m_Radius; }

    // 距離の二乗で判定する（sqrt を省いて高速化）
    bool Overlaps(const SphereCollider& other) const;

private:
    float m_Radius = 1.0f;
};
