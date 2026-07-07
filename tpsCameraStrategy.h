#pragma once
#include "cameraStrategy.h"

class TpsCameraStrategy : public CameraStrategy
{
public:
    void         OnEnter(const Vector3& camPos, const Vector3& target) override;
    CameraOutput Update(const CameraContext& ctx) override;

private:
    Vector3 m_CameraPos = {};  // Lerp追従用の現在座標
};
