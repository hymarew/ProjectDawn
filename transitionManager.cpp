#include "transitionManager.h"
#include "fadeTransition.h"
#include "wipeTransition.h"
#include "circleTransition.h"
#include "slideTransition.h"
#include "curtainTransition.h"
#include "mosaicTransition.h"
#include "blurTransition.h"
#include "distortionTransition.h"
#include "pixelDissolveTransition.h"

TransitionManager g_TransitionManager;

// ---------------------------------------------------------
// Init : 対応するトランジションをここで登録する。
// 新しい種類を増やすときはこの関数に1行足すだけでよい（switch文は使わない）。
// ---------------------------------------------------------
void TransitionManager::Init()
{
    Register<FadeTransition>(TransitionType::Fade);
    Register<WipeTransition>(TransitionType::Wipe);
    Register<CircleTransition>(TransitionType::Circle);
    Register<SlideTransition>(TransitionType::Slide);
    Register<CurtainTransition>(TransitionType::Curtain);
    Register<MosaicTransition>(TransitionType::Mosaic);
    Register<BlurTransition>(TransitionType::Blur);
    Register<DistortionTransition>(TransitionType::Distortion);
    Register<PixelDissolveTransition>(TransitionType::PixelDissolve);
}

void TransitionManager::Uninit()
{
    for (auto& pair : m_Registry)
        pair.second->Uninit();

    m_Registry.clear();
    m_Current = nullptr;
}

// ---------------------------------------------------------
// Play : type に対応するトランジションを再生する
// ---------------------------------------------------------
void TransitionManager::Play(TransitionType type, TransitionMode mode, float duration)
{
    auto it = m_Registry.find(type);
    if (it == m_Registry.end()) return; // 未登録の種別は何もしない

    m_Current = it->second.get();
    m_Current->Start(mode, duration);
}

void TransitionManager::Stop()
{
    m_Current = nullptr;
}

void TransitionManager::Update(float dt)
{
    if (m_Current) m_Current->Update(dt);
}

void TransitionManager::Draw()
{
    // In完了後（完全に元へ戻った状態）は描画しない。
    // これが無いとMosaic/Blur/Distortion等が永遠に画面を覆い続けてしまう。
    if (m_Current && m_Current->IsVisible()) m_Current->Draw();
}
