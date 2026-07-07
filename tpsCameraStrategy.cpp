#include "tpsCameraStrategy.h"
#include "GameConfig.h"
#include <cmath>

void TpsCameraStrategy::OnEnter(const Vector3& camPos, const Vector3& target)
{
    m_CameraPos = camPos;
}

CameraOutput TpsCameraStrategy::Update(const CameraContext& ctx)
{
    float yaw   = ctx.yaw;
    float pitch = ctx.pitch;

    // カメラ目標位置: プレイヤーの後方 + 上方
    Vector3 targetPos = ctx.playerPos + Vector3(
        -sinf(yaw) * GameConfig::Camera::TPS_BACK_DIST,
         GameConfig::Camera::TPS_HEIGHT_EDF,
        -cosf(yaw) * GameConfig::Camera::TPS_BACK_DIST);

    // スムース追従（Lerp）
    float s = GameConfig::Camera::TPS_FOLLOW_SPEED;
    m_CameraPos = m_CameraPos * (1.0f - s) + targetPos * s;

    // 注視点: Pitchを込みの3Dエイム方向（クロスヘア中央と一致）
    Vector3 aimDir = Vector3(
        sinf(yaw) * cosf(pitch),
        sinf(pitch),
        cosf(yaw) * cosf(pitch));

    Vector3 lookTarget = ctx.playerPos
        + Vector3(0.0f, GameConfig::Camera::TPS_LOOKAT_UP, 0.0f)
        + aimDir * GameConfig::Camera::TPS_LOOKAT_AHEAD;

    CameraOutput out;
    out.position = m_CameraPos;
    out.target   = lookTarget;
    return out;
}
