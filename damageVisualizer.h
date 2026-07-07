#pragma once
#include <vector>
#include "vector3.h"

class Camera;

enum class DamageDisplayMode
{
    POPUP,       // ヒット位置に浮かせて表示（従来）
    ACCUMULATE,  // ダメージを右下に累積表示、1秒更新なしでリセット
};

// =====================================================
// DamageVisualizer
// 使い方:
//   g_DamageVisualizer.Add(worldPos, damage);  // ヒット時に呼ぶ
//   g_DamageVisualizer.Update(dt);             // Manager::Update から
//   g_DamageVisualizer.Draw(camera);           // ImGui::Render() の直前に呼ぶ
// =====================================================
class DamageVisualizer
{
public:
    void Add(const Vector3& worldPos, float damage);
    void Update(float dt);
    void Draw(Camera* cam, float fontSize = 20.0f);

    DamageDisplayMode GetMode() const              { return m_Mode; }
    void              SetMode(DamageDisplayMode m) { m_Mode = m; }

private:
    // --- POPUPモード用 ---
    struct DamagePopup
    {
        Vector3 worldPos;
        float   damage  = 0.0f;
        float   timer   = 0.0f;
        static constexpr float LIFETIME = 0.8f;
        bool IsAlive() const { return timer < LIFETIME; }
    };

    DamageDisplayMode        m_Mode   = DamageDisplayMode::POPUP;
    std::vector<DamagePopup> m_Popups;

    // --- ACCUMULATEモード用 ---
    float m_AccumTotal     = 0.0f;  // 累積ダメージ合計
    float m_AccumIdleTimer = 0.0f;  // 最後のダメージからの経過時間
    static constexpr float ACCUM_RESET_SEC = 1.0f; // これ以上更新なしでリセット
};

extern DamageVisualizer g_DamageVisualizer;
