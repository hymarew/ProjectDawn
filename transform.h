#pragma once
#include <DirectXMath.h>
#include "vector3.h"

using namespace DirectX;

// =====================================================
// Transform : SpaceRift専用の軽量なワールド変換
//
// GameObjectはコンポーネントリストやレイヤーソート等、多くの機能を持つ重い基底クラスで、
// SpaceRiftはゲームロジックに属さない独立したデバッグ用エフェクトのため、
// あえてGameObjectを継承せず「位置・回転・スケール→ワールド行列」だけを担う。
// =====================================================
class Transform
{
public:
    Vector3 Position{ 0.0f, 0.0f, 0.0f };
    Vector3 Rotation{ 0.0f, 0.0f, 0.0f }; // ラジアン (Pitch=X, Yaw=Y, Roll=Z)
    Vector3 Scale{ 1.0f, 1.0f, 1.0f };

    XMMATRIX GetWorldMatrix() const
    {
        XMMATRIX s = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
        XMMATRIX r = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
        XMMATRIX t = XMMatrixTranslation(Position.x, Position.y, Position.z);
        return s * r * t;
    }
};
