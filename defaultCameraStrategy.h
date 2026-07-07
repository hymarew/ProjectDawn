#pragma once
#include "cameraStrategy.h"

class DefaultCameraStrategy : public CameraStrategy
{
public:
    void         OnEnter(const Vector3& camPos, const Vector3& target) override;
    CameraOutput Update(const CameraContext& ctx) override;

private:
    Vector3 m_SmoothTarget = {};
};
