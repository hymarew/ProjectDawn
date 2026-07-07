#include "main.h"
#include "fadeManager.h"

FadeManager g_FadeManager;

void FadeManager::StartFadeOut(float duration)
{
    m_State    = FadeState::FadeOut;
    m_Duration = (duration > 0.0f) ? duration : DEFAULT_FADE_OUT;
    m_Timer    = 0.0f;
    m_Alpha    = 0.0f;
    m_Finished = false;
}

void FadeManager::StartFadeIn(float duration)
{
    m_State    = FadeState::FadeIn;
    m_Duration = (duration > 0.0f) ? duration : DEFAULT_FADE_IN;
    m_Timer    = 0.0f;
    m_Alpha    = 1.0f;
    m_Finished = false;
}

void FadeManager::Update(float dt)
{
    m_Finished = false;

    if (m_State == FadeState::None) return;

    m_Timer += dt;
    float t = (m_Duration > 0.0f) ? (m_Timer / m_Duration) : 1.0f;
    if (t > 1.0f) t = 1.0f;

    switch (m_State)
    {
    case FadeState::FadeOut:
        m_Alpha = t;
        if (t >= 1.0f)
        {
            m_Alpha    = 1.0f;
            m_Finished = true;
            m_State    = FadeState::None;
        }
        break;

    case FadeState::FadeIn:
        m_Alpha = 1.0f - t;
        if (t >= 1.0f)
        {
            m_Alpha    = 0.0f;
            m_Finished = true;
            m_State    = FadeState::None;
        }
        break;

    default:
        break;
    }
}

void FadeManager::Draw()
{
    if (m_Alpha <= 0.0f) return;

    // ForegroundDrawList を使うことでポーズメニューを含む全UIの前面に描画される
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImU32 col = IM_COL32(0, 0, 0, (int)(m_Alpha * 255.0f));
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        col);
}
