#pragma once
#include "weatherEmitter.h"

// ===================================================
// SnowEmitter : 雪の生成・移動・寿命管理
//
// ゆっくり落下しながら左右にゆらゆら揺れ、回転する丸い雪片。
// 揺れは「風で流される基準位置(Anchor) + sin波によるオフセット」で表現する。
// プールはInit時に一括確保し、以後 new/delete なしで再利用する。
// ===================================================
class SnowEmitter : public WeatherEmitter
{
public:
    void Init(int maxParticles);

    void Update(float dt, const Vector3& cameraPos, float groundY) override;
    void FillInstances(std::vector<WeatherInstance>& out,
                       const Vector3& cameraPos,
                       const Vector3& cameraRight) const override;

    int  GetActiveCount() const override { return m_ActiveCount; }
    bool IsEnabled()      const override { return m_Params.Enabled; }

    SnowParams& GetParams() { return m_Params; } // ImGuiから直接編集する

private:
    // 更新フローの各段階（Update から順に呼ばれる）
    void Spawn(float dt, const Vector3& cameraPos);
    void Move(float dt, const Vector3& cameraPos);
    void UpdateLife(float dt, float groundY);

    void ActivateOne(SnowParticle& p, const Vector3& cameraPos);

    SnowParams                m_Params;
    std::vector<SnowParticle> m_Pool;        // 固定長プール（再確保しない）
    int                       m_NextFree    = 0;
    int                       m_ActiveCount = 0;
    float                     m_Time        = 0.0f; // 揺れのsin波に使う積算時間
};
