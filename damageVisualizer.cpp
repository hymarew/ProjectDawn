#include "damageVisualizer.h"
#include "camera.h"
#include "main.h"

#include <algorithm>
using namespace DirectX;

DamageVisualizer g_DamageVisualizer;

void DamageVisualizer::Add(const Vector3& worldPos, float damage)
{
    // POPUPモード用
    DamagePopup p;
    p.worldPos = worldPos;
    p.damage   = damage;
    p.timer    = 0.0f;
    m_Popups.push_back(p);

    // ACCUMULATEモード用: 累積に加算してアイドルタイマーをリセット
    m_AccumTotal     += damage;
    m_AccumIdleTimer  = 0.0f;
}

void DamageVisualizer::Update(float dt)
{
    // POPUP
    for (auto& p : m_Popups)
        p.timer += dt;
    m_Popups.erase(
        std::remove_if(m_Popups.begin(), m_Popups.end(),
            [](const DamagePopup& p) { return !p.IsAlive(); }),
        m_Popups.end());

    // ACCUMULATE: 1秒間ダメージがなければリセット
    if (m_AccumTotal > 0.0f)
    {
        m_AccumIdleTimer += dt;
        if (m_AccumIdleTimer >= ACCUM_RESET_SEC)
        {
            m_AccumTotal     = 0.0f;
            m_AccumIdleTimer = 0.0f;
        }
    }
}

void DamageVisualizer::Draw(Camera* cam, float fontSize)
{
    if (!cam) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    if (m_Mode == DamageDisplayMode::POPUP)
    {
        if (m_Popups.empty()) return;

        XMMATRIX vp = cam->GetViewMatrix() * cam->GetProjectionMatrix();

        for (const auto& p : m_Popups)
        {
            XMVECTOR wp   = XMVectorSet(p.worldPos.x, p.worldPos.y + 1.5f, p.worldPos.z, 1.0f);
            XMVECTOR clip = XMVector4Transform(wp, vp);

            float w = XMVectorGetW(clip);
            if (w <= 0.0f) continue;

            float sx = ( XMVectorGetX(clip) / w * 0.5f + 0.5f) * SCREEN_WIDTH;
            float sy = (-XMVectorGetY(clip) / w * 0.5f + 0.5f) * SCREEN_HEIGHT;

            float ratio = p.timer / DamagePopup::LIFETIME;
            sy -= ratio * 40.0f;

            ImU32 alpha = (ImU32)((1.0f - ratio) * 255);
            ImU32 color = IM_COL32(255, 220, 50, alpha);

            char buf[16];
            sprintf_s(buf, "%.0f", p.damage);
            dl->AddText(ImGui::GetFont(), fontSize, ImVec2(sx, sy), color, buf);
        }
    }
    else // ACCUMULATE
    {
        if (m_AccumTotal <= 0.0f) return;

        char buf[32];
        sprintf_s(buf, "DMG: %.0f", m_AccumTotal);

        constexpr float MARGIN   = 16.0f;
        float           dispSize = fontSize * 1.5f;

        ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(dispSize, FLT_MAX, 0.0f, buf);
        float sx = SCREEN_WIDTH  - textSize.x - MARGIN;
        float sy = SCREEN_HEIGHT - textSize.y - MARGIN;

        // 影（視認性向上）
        dl->AddText(ImGui::GetFont(), dispSize,
            ImVec2(sx + 2, sy + 2), IM_COL32(0, 0, 0, 200), buf);
        dl->AddText(ImGui::GetFont(), dispSize,
            ImVec2(sx, sy), IM_COL32(255, 80, 80, 255), buf);
    }
}
