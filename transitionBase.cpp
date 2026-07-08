#include "transitionBase.h"

// ---------------------------------------------------------
// Start : 再生開始。timer/progress をリセットして再生状態にする
// ---------------------------------------------------------
void TransitionBase::Start(TransitionMode mode, float duration)
{
    m_Mode     = mode;
    m_Duration = (duration > 0.0f) ? duration : m_DefaultDuration;
    m_Timer    = 0.0f;
    m_Progress = 0.0f;
    m_Playing  = true;
    m_Finished = false;
}

// ---------------------------------------------------------
// Update : 経過時間から progress(0→1) を計算する共通処理
// ---------------------------------------------------------
void TransitionBase::Update(float dt)
{
    // IsFinished() は「完了したその1フレームだけ」true にしたいので、
    // 毎フレーム先頭で一旦falseに戻す（FadeManagerと同じ考え方）
    m_Finished = false;

    if (!m_Playing) return;

    m_Timer += dt;
    m_Progress = (m_Duration > 0.0f) ? (m_Timer / m_Duration) : 1.0f;

    if (m_Progress >= 1.0f)
    {
        m_Progress = 1.0f;
        m_Playing  = false;
        m_Finished = true;
    }
}
