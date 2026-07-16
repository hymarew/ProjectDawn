#include "riftController.h"

void RiftController::Init()
{
    m_Time    = 0.0f;
    m_Enabled = true;
}

void RiftController::Update(float dt, RiftMaterial& material)
{
    if (!m_Enabled) return;

    m_Time += dt;
    material.Time = m_Time;
}
