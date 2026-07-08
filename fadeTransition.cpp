#include "fadeTransition.h"

void FadeTransition::Init()
{
    m_Quad.Init();
}

void FadeTransition::Uninit()
{
    m_Quad.Uninit();
}

// ---------------------------------------------------------
// Draw : Out(通常→覆う)なら progress そのまま、In(覆う→通常)なら反転してalphaにする。
// 再生が終わった後も最後の progress を保持し続けるので、
// 「FadeOut完了直後、SceneManagerがまだシーンを切り替えていない間も画面が覆われたまま」
// という挙動が自然に成立する（FadeManagerの alpha 据え置きと同じ考え方）。
// ---------------------------------------------------------
void FadeTransition::Draw()
{
    float progress = GetEasedProgress();
    float alpha = (GetMode() == TransitionMode::Out) ? progress : (1.0f - progress);

    XMFLOAT4 color = m_Color;
    color.w = alpha;
    m_Quad.Draw(color);
}
