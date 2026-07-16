#pragma once
#include "weatherEmitter.h"

// ===================================================
// RainEmitter : 雨の生成・移動・寿命管理
//
// 高速で真下へ落下し、風でわずかに横へ流れる細長い雨粒。
// プールはInit時に一括確保し、以後 new/delete なしで再利用する。
// 地面到達・寿命切れで非アクティブ化し、Spawnが空きスロットを再利用する。
// ===================================================
class RainEmitter : public WeatherEmitter
{
public:
    // maxParticles: プールの固定サイズ（この数だけInit時に一括確保する）
    void Init(int maxParticles);

    void Update(float dt, const Vector3& cameraPos, float groundY) override;
    void FillInstances(std::vector<WeatherInstance>& out,
                       const Vector3& cameraPos,
                       const Vector3& cameraRight) const override;

    int  GetActiveCount() const override { return m_ActiveCount; }
    bool IsEnabled()      const override { return m_Params.Enabled; }

    RainParams& GetParams() { return m_Params; } // ImGuiから直接編集する

private:
    // 更新フローの各段階（Update から順に呼ばれる）
    void Spawn(float dt, const Vector3& cameraPos);
    void Move(float dt, const Vector3& cameraPos);
    void UpdateLife(float dt, float groundY);

    // 1粒を生成領域内のランダム位置でアクティブ化する
    void ActivateOne(RainParticle& p, const Vector3& cameraPos);

    RainParams                m_Params;
    std::vector<RainParticle> m_Pool;        // 固定長プール（再確保しない）
    int                       m_NextFree    = 0; // 空きスロット検索の開始位置（リング走査）
    int                       m_ActiveCount = 0;
};
