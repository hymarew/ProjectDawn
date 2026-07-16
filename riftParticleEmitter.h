#pragma once
#include "vector3.h"

// =====================================================
// RiftParticleEmitter : SpaceRiftの中心から青/白のエネルギー粒子を放射状に放つ
//
// 要求仕様のクラス図における"ParticleEmitter"に対応する。既存のグローバルクラス
// ParticleEmitter(particleEmitter.h)と名前が衝突するため RiftParticleEmitter とした。
// 実体の生成・寿命管理は既存のGPUインスタンシング済みパーティクル基盤
// (ParticleManager/ParticleEmitter)にそのまま委譲し、このクラスは
// 「一定間隔で青/白どちらを何個放つか」という発生ロジックのみを担当する。
// =====================================================
class RiftParticleEmitter
{
public:
    void Init();

    // position: 裂け目の中心位置。enabled=falseの間は放出しない
    void Update(float dt, const Vector3& position, bool enabled);

private:
    float m_Timer = 0.0f; // 次の放出までのカウントダウン（秒）
};
