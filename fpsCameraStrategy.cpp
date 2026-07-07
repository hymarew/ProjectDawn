#include "fpsCameraStrategy.h"
#include "GameConfig.h"
#include <cmath>

CameraOutput FpsCameraStrategy::Update(const CameraContext& ctx)
{
    CameraOutput out;
    out.position = ctx.playerPos + Vector3(0.0f, GameConfig::Camera::FPS_HEIGHT, 0.0f);
    out.target   = out.position + Vector3(
        sinf(ctx.yaw),
        sinf(ctx.pitch),
        cosf(ctx.yaw));
    return out;
}
