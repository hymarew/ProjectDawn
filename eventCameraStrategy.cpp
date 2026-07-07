#include "eventCameraStrategy.h"
#include "GameConfig.h"

void EventCameraStrategy::OnEnter(const Vector3& camPos, const Vector3& target)
{
    m_CameraPos = camPos;
}

CameraOutput EventCameraStrategy::Update(const CameraContext& ctx)
{
    // ターゲットが未設定の場合はプレイヤー位置にフォールバック
    Vector3 focusPos = ctx.eventTarget
        ? ctx.eventTarget->GetPosition()
        : ctx.playerPos;

    // ターゲット背後のオフセット位置にLerpで移動
    Vector3 goalPos = focusPos + m_Offset;
    float s = GameConfig::Camera::TPS_FOLLOW_SPEED;
    m_CameraPos = m_CameraPos * (1.0f - s) + goalPos * s;

    CameraOutput out;
    out.position = m_CameraPos;
    out.target   = focusPos;
    return out;
}
