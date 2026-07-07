#include "defaultCameraStrategy.h"
#include "GameConfig.h"
#include <cmath>

void DefaultCameraStrategy::OnEnter(const Vector3& camPos, const Vector3& target)
{
    m_SmoothTarget = target;
}

CameraOutput DefaultCameraStrategy::Update(const CameraContext& ctx)
{
    float t = GameConfig::Camera::SMOOTH_T;

    // 注視点をプレイヤー頭上にLerpで滑らかに追従
    Vector3 goal = ctx.playerPos + Vector3(0.0f, GameConfig::Camera::DEFAULT_TARGET_Y, 0.0f);
    m_SmoothTarget = m_SmoothTarget * (1.0f - t) + goal * t;

    CameraOutput out;
    out.target   = m_SmoothTarget;
    out.position = m_SmoothTarget + Vector3(
        -sinf(ctx.yaw) * GameConfig::Camera::DEFAULT_DISTANCE,
         GameConfig::Camera::DEFAULT_HEIGHT,
        -cosf(ctx.yaw) * GameConfig::Camera::DEFAULT_DISTANCE);
    return out;
}
