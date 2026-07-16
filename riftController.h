#pragma once
#include "riftMaterial.h"

// =====================================================
// RiftController : SpaceRiftの時間経過・発光の脈動・ON/OFF状態を管理する
//
// 実際の脈動の数式(sin(time)等)はGPU側(riftPS.hlsl)で計算するため、
// このクラスの責務は「経過時間を積算し、RiftMaterialへ渡す」ことと
// 「有効/無効の状態を保持する」ことに絞る。
// =====================================================
class RiftController
{
public:
    void Init();

    // 有効な間だけ時間を進め、material.Time へ反映する
    void Update(float dt, RiftMaterial& material);

    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

    float GetTime() const { return m_Time; }

private:
    float m_Time    = 0.0f;
    bool  m_Enabled = true;
};
