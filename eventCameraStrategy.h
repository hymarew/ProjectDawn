#pragma once
#include "cameraStrategy.h"
#include "gameObject.h"

// =====================================================
// EventCameraStrategy
// 特定の GameObject を映し続けるカメラ。
// 使い方:
//   camera->SetEventTarget(bossObj);
//   camera->SetMode(CameraMode::EVENT);
// =====================================================
class EventCameraStrategy : public CameraStrategy
{
public:
    // オフセット設定（デフォルト: 後方5m・上3m）
    void SetOffset(const Vector3& offset) { m_Offset = offset; }

    void         OnEnter(const Vector3& camPos, const Vector3& target) override;
    CameraOutput Update(const CameraContext& ctx) override;

private:
    Vector3 m_Offset    = { 0.0f, 3.0f, -5.0f };
    Vector3 m_CameraPos = {};
};
