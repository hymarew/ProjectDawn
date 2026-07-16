#include "riftParticleEmitter.h"
#include "particleManager.h"
#include <cstdlib>

namespace
{
    constexpr float INTERVAL        = 0.08f; // この間隔で1バーストずつ放出する
    constexpr int   COUNT_PER_BURST = 3;     // 1回の放出で何個まとめて出すか
}

void RiftParticleEmitter::Init()
{
    m_Timer = 0.0f;
}

void RiftParticleEmitter::Update(float dt, const Vector3& position, bool enabled)
{
    if (!enabled) return;

    m_Timer -= dt;
    if (m_Timer > 0.0f) return;
    m_Timer += INTERVAL;

    // 青・白をランダムに選んで中心から放射状にバースト放出する
    ParticleSetting s = (rand() % 2 == 0) ? ParticlePreset::RiftEnergyBlue()
                                           : ParticlePreset::RiftEnergyWhite();
    s.BurstCount = COUNT_PER_BURST;
    ParticleManager::GetInstance().Emit(s, position);
}
