#pragma once
#include "cameraStrategy.h"

class FpsCameraStrategy : public CameraStrategy
{
public:
    CameraOutput Update(const CameraContext& ctx) override;
};
