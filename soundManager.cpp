// ===================================================
// soundManager.cpp
// SE・BGMの再生を一元管理するグローバルクラスの実装
// ===================================================

#include "soundManager.h"

SoundManager g_SoundManager;

// ---------------------------------------------------------
// Init : 全SE・BGMをロードする
// ---------------------------------------------------------
void SoundManager::Init()
{
    // GameObjectに紐付かない単発の音源として Audio(nullptr) で構築する
    m_SEShotgunFire    = std::make_unique<Audio>(nullptr);
    m_SERocketLauncher = std::make_unique<Audio>(nullptr);
    m_SEExplosion1     = std::make_unique<Audio>(nullptr);
    m_BgmMenu          = std::make_unique<Audio>(nullptr);
    m_BgmGame          = std::make_unique<Audio>(nullptr);

    m_SEShotgunFire   ->Load(L"asset\\audio\\se\\shotgunFire.wav");
    m_SERocketLauncher->Load(L"asset\\audio\\se\\rocketLauncher.wav");
    m_SEExplosion1    ->Load(L"asset\\audio\\se\\explosion1.wav");
    m_BgmMenu         ->Load(L"asset\\audio\\bgm\\bgmMenu.wav");
    m_BgmGame         ->Load(L"asset\\audio\\bgm\\bgmGame.wav");

    m_CurrentBgm = BgmType::None;
}

// ---------------------------------------------------------
// Uninit : 保持している Audio をすべて解放する
// ---------------------------------------------------------
void SoundManager::Uninit()
{
    m_SEShotgunFire.reset();
    m_SERocketLauncher.reset();
    m_SEExplosion1.reset();
    m_BgmMenu.reset();
    m_BgmGame.reset();
}

// ---------------------------------------------------------
// PlaySE : 効果音を1回再生する
// ---------------------------------------------------------
void SoundManager::PlaySE(SEType type)
{
    //switch (type)
    //{
    //case SEType::ShotgunFire:    if (m_SEShotgunFire)    m_SEShotgunFire->Play(false);    break;
    //case SEType::RocketLauncher: if (m_SERocketLauncher) m_SERocketLauncher->Play(false); break;
    //case SEType::Explosion1:     if (m_SEExplosion1)     m_SEExplosion1->Play(false);     break;
    //}
}

// ---------------------------------------------------------
// PlayBgm : BGMを切り替える（同じ種類なら継続再生のため何もしない）
// ---------------------------------------------------------
void SoundManager::PlayBgm(BgmType type)
{
    //if (type == m_CurrentBgm) return;

    //// 現在再生中のBGMを止めてから次のBGMを再生する
    //if (m_CurrentBgm == BgmType::Menu && m_BgmMenu) m_BgmMenu->Stop();
    //if (m_CurrentBgm == BgmType::Game && m_BgmGame) m_BgmGame->Stop();

    //switch (type)
    //{
    //case BgmType::Menu: if (m_BgmMenu) m_BgmMenu->Play(true); break;
    //case BgmType::Game: if (m_BgmGame) m_BgmGame->Play(true); break;
    //default: break;
    //}

    //m_CurrentBgm = type;
}

// ---------------------------------------------------------
// StopBgm : BGMを停止する
// ---------------------------------------------------------
void SoundManager::StopBgm()
{
    if (m_CurrentBgm == BgmType::Menu && m_BgmMenu) m_BgmMenu->Stop();
    if (m_CurrentBgm == BgmType::Game && m_BgmGame) m_BgmGame->Stop();
    m_CurrentBgm = BgmType::None;
}

// ---------------------------------------------------------
// SetBgmVolume / SetSEVolume : 音量を全ボイスへ即時反映する
// ---------------------------------------------------------
void SoundManager::SetBgmVolume(float volume)
{
    m_BgmVolume = volume;
    if (m_BgmMenu) m_BgmMenu->SetVolume(volume);
    if (m_BgmGame) m_BgmGame->SetVolume(volume);
}

void SoundManager::SetSEVolume(float volume)
{
    m_SEVolume = volume;
    if (m_SEShotgunFire)    m_SEShotgunFire->SetVolume(volume);
    if (m_SERocketLauncher) m_SERocketLauncher->SetVolume(volume);
    if (m_SEExplosion1)     m_SEExplosion1->SetVolume(volume);
}
