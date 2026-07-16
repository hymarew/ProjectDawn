#pragma once
#include "audio.h"
#include <memory>

// ===================================================
// soundManager.h
// SE・BGMの再生を一元管理するグローバルクラス
//
// 【役割】
//   - 効果音（SE）：3種を起動時に1回だけロードし、鳴らすたびに使い回す
//   - BGM：シーン種別（Menu系 / Game系）に応じて自動的に切り替える。
//     同じ種類のBGMが指定された場合は再生し直さない（継続再生）
//
// 【使い方】
//   Manager::Init()  : g_SoundManager.Init()
//   Manager::Uninit(): g_SoundManager.Uninit()
//   SE再生            : g_SoundManager.PlaySE(SEType::ShotgunFire)
//   BGM切り替え       : g_SoundManager.PlayBgm(BgmType::Menu)
// ===================================================

// 効果音の種別
enum class SEType
{
    ShotgunFire,     // ARの発射音（提供素材の都合上「ショットガン発射」を使用）
    RocketLauncher,  // ロケットランチャーの発射音
    Explosion1,      // ロケット弾の着弾爆発音
};

// BGMの種別
enum class BgmType
{
    None, // 未再生
    Menu, // タイトル・メニュー・武器選択・ステージ選択・リザルト等
    Game, // ゲームプレイ本編
};

class SoundManager
{
public:
    // 全SE・BGMをロードする（Audio::InitMaster() の後に呼ぶこと）
    void Init();

    // 保持している Audio をすべて解放する（Audio::UninitMaster() の前に呼ぶこと）
    void Uninit();

    // 効果音を1回再生する（多重再生はせず、同じSourceVoiceを鳴らし直す）
    void PlaySE(SEType type);

    // BGMを再生する。現在再生中の種類と同じ場合は何もしない（継続再生）
    void PlayBgm(BgmType type);

    // BGMを停止する
    void StopBgm();

    // 音量設定（0.0〜1.0）。OptionsMenu から呼ばれ、再生中の音にも即時反映される
    void  SetBgmVolume(float volume);
    void  SetSEVolume(float volume);
    float GetBgmVolume() const { return m_BgmVolume; }
    float GetSEVolume()  const { return m_SEVolume;  }

private:
    // SE用（GameObjectに紐付かない用途のため Audio(nullptr) で構築する）
    std::unique_ptr<Audio> m_SEShotgunFire;
    std::unique_ptr<Audio> m_SERocketLauncher;
    std::unique_ptr<Audio> m_SEExplosion1;

    // BGM用（Menu系・Game系の2本を切り替えて使う）
    std::unique_ptr<Audio> m_BgmMenu;
    std::unique_ptr<Audio> m_BgmGame;

    BgmType m_CurrentBgm = BgmType::None;

    float m_BgmVolume = 1.0f;  // 起動時に save.json の設定値で上書きされる
    float m_SEVolume  = 1.0f;
};

// グローバルインスタンス（soundManager.cpp で定義）
extern SoundManager g_SoundManager;
