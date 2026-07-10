#pragma once
#include "particleSetting.h"
#include "particleEmitter.h"
#include "particleRenderer.h"
#include <vector>
#include <memory>

// パーティクルシステムの統計情報（デバッグUI表示用）
struct ParticleStats
{
    int   ActiveCount = 0;    // 現在アクティブなパーティクル数
    int   UsedSlots   = 0;    // 現在の走査範囲（ハイウォーターマーク）
    int   DrawCalls   = 0;    // 直近フレームのドローコール数
    float UpdateMs    = 0.0f; // CPUシミュレーションにかかった時間（ミリ秒）
    float DrawMs      = 0.0f; // 描画準備＋発行にかかったCPU時間（ミリ秒）
};

// 全パーティクルと全エミッタを一元管理するシングルトンクラス
// 呼び出し側は Emit(EffectType, position) の1行でエフェクトを発生させられる
// パーティクルのプールはこのクラスが単一で保持する（エミッタは持たない）
class ParticleManager
{
public:
    static ParticleManager& GetInstance()
    {
        static ParticleManager s_Instance;
        return s_Instance;
    }

    void Init();
    void Uninit();
    void Update(float dt);
    void Draw();

    // エフェクトを発生させる。ゲームシーンやオブジェクトから呼び出す
    void Emit(EffectType type, Vector3 position);

    // プリセットを直接指定して発生させる（生成数などをパラメータ化した演出用）
    void Emit(const ParticleSetting& setting, Vector3 position);

    // 発光フラッシュ・火球・火花・デブリ・煙・爆風リングを一括で発生させる爆発演出
    void EmitBigExplosion(Vector3 position);

    // スコーピオン被弾演出（火花・装甲片・粉・衝撃リングの合成）
    // 「硬い外骨格に弾丸が弾かれる」印象。爆発は使わない。
    void EmitScorpionHit(Vector3 position);

    // スコーピオン撃破演出（被弾演出の強化版。装甲が限界を迎えて砕け散るイメージ）
    void EmitScorpionDeath(Vector3 position);

    // 回復演出（緑のキラキラした光の粒がふわっと舞い上がる）
    void EmitHeal(Vector3 position);

    // 画面全体を一瞬明るくするフラッシュ（ImGui::Render() の直前に呼ぶ）
    void DrawScreenFlash();

    // 直近フレームの統計（デバッグUI表示用）
    const ParticleStats& GetStats() const { return m_Stats; }

private:
    // 画面フラッシュ発生。EmitBigExplosion から呼ばれる
    void TriggerScreenFlash(float duration);

    // Field オブジェクトの高さを地面座標として取得する（未発見時は 0.0f）
    float GetGroundY();

    ParticleManager()  = default;
    ~ParticleManager() = default;

    // コピー・ムーブ禁止（シングルトン）
    ParticleManager(const ParticleManager&)            = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    // 同時に存在できるパーティクルの上限（調整は GameConfig::Particle::POOL_SIZE で行う）
    static constexpr int POOL_SIZE = GameConfig::Particle::POOL_SIZE;

    // グローバルプール（エミッタ間で共有）
    // 100万個 × 約150バイト ≒ 150MB になるため、静的領域ではなくヒープに置く
    std::unique_ptr<ParticleData[]> m_Pool;
    int m_NextFree = 0; // リングバッファの次の書き込み位置

    // ハイウォーターマーク: これまでに書き込んだことのあるスロット数（上限 POOL_SIZE）。
    // 更新・描画の走査はここまでで打ち切れるため、プール上限を100万にしても
    // 通常プレイ（数千個程度）の走査コストは増えない
    int m_UsedSlots = 0;

    std::vector<std::unique_ptr<ParticleEmitter>> m_Emitters; // アクティブなエミッタ一覧

    ParticleRenderer m_Renderer;

    Vector3 m_Gravity; // 毎フレーム加算する重力加速度ベクトル

    // 地面座標のキャッシュ（Field オブジェクトが見つかった時点で確定する）
    float m_GroundY       = 0.0f;
    bool  m_GroundYCached = false;

    // 画面フラッシュの状態
    float m_FlashTimer    = 0.0f; // 残り時間（秒）
    float m_FlashDuration = 0.0f; // 発生時の合計時間（秒）

    ParticleStats m_Stats; // 直近フレームの統計（デバッグUI表示用）
};
