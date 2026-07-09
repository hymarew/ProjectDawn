#include "damageEffectManager.h"
#include "GameConfig.h"
#include <cstdlib>
#include <cmath>

DamageEffectManager g_DamageEffectManager;

static float LerpF(float a, float b, float t) { return a + (b - a) * t; }

// ---------------------------------------------------------
// Update : 毎フレーム呼ぶ。各タイマーを進め、シェイクオフセットと
// 低HP心拍の状態を更新する。
// ---------------------------------------------------------
void DamageEffectManager::Update(float dt, float hpRatio)
{
    using namespace GameConfig::DamageEffect;

    // ---- HUDシェイク ----
    if (m_ShakeTimer > 0.0f)
    {
        m_ShakeTimer -= dt;
        if (m_ShakeTimer < 0.0f) m_ShakeTimer = 0.0f;

        // 残り時間の割合だけ減衰させながら、毎フレームランダムな方向へオフセットする
        float decay = m_ShakeTimer / SHAKE_DURATION;
        m_ShakeOffset.x = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * SHAKE_MAX_OFFSET * decay;
        m_ShakeOffset.y = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * SHAKE_MAX_OFFSET * decay;
    }
    else
    {
        m_ShakeOffset = {};
    }

    // ---- HPフラッシュ / 被弾ビネット ----
    if (m_HpFlashTimer > 0.0f)
    {
        m_HpFlashTimer -= dt;
        if (m_HpFlashTimer < 0.0f) m_HpFlashTimer = 0.0f;
    }
    if (m_HitVignetteTimer > 0.0f)
    {
        m_HitVignetteTimer -= dt;
        if (m_HitVignetteTimer < 0.0f) m_HitVignetteTimer = 0.0f;
    }

    // ---- 低HP心拍 ----
    // HPが30%以上へ回復したら停止する（位相もリセットし、次に低HPになったとき最初から鼓動させる）
    m_LowHpActive = (hpRatio <= LOW_HP_RATIO) && (hpRatio > 0.0f);
    if (m_LowHpActive)
        m_HeartbeatTime += dt;
    else
        m_HeartbeatTime = 0.0f;
}

// ---------------------------------------------------------
// OnDamaged : プレイヤーが被弾した瞬間に呼ぶ。各演出を再生開始する。
// ---------------------------------------------------------
void DamageEffectManager::OnDamaged()
{
    using namespace GameConfig::DamageEffect;
    m_ShakeTimer       = SHAKE_DURATION;
    m_HpFlashTimer     = HP_FLASH_DURATION;
    m_HitVignetteTimer = HIT_VIGNETTE_DURATION;
}

// ---------------------------------------------------------
// GetHpFlashRatio : 0(通常色)〜1(真っ赤)
// ---------------------------------------------------------
float DamageEffectManager::GetHpFlashRatio() const
{
    using namespace GameConfig::DamageEffect;
    if (HP_FLASH_DURATION <= 0.0f) return 0.0f;
    return m_HpFlashTimer / HP_FLASH_DURATION;
}

// ---------------------------------------------------------
// GetVignetteAlpha : 被弾フラッシュ（0%→35%→0%）と低HP心拍のうち大きい方を返す
// ---------------------------------------------------------
float DamageEffectManager::GetVignetteAlpha() const
{
    using namespace GameConfig::DamageEffect;

    // ---- 被弾フラッシュ: 立ち上がり→減衰の三角形カーブ ----
    float hitAlpha = 0.0f;
    if (m_HitVignetteTimer > 0.0f && HIT_VIGNETTE_DURATION > 0.0f)
    {
        float elapsedT = 1.0f - (m_HitVignetteTimer / HIT_VIGNETTE_DURATION); // 0→1
        if (elapsedT < HIT_VIGNETTE_RISE_RATIO)
        {
            hitAlpha = LerpF(0.0f, HIT_VIGNETTE_MAX_ALPHA, elapsedT / HIT_VIGNETTE_RISE_RATIO);
        }
        else
        {
            float fallT = (elapsedT - HIT_VIGNETTE_RISE_RATIO) / (1.0f - HIT_VIGNETTE_RISE_RATIO);
            hitAlpha = LerpF(HIT_VIGNETTE_MAX_ALPHA, 0.0f, fallT);
        }
    }

    // ---- 低HP心拍: 周期ごとに鋭く立ち上がって沈む鼓動カーブ ----
    float heartbeatAlpha = 0.0f;
    if (m_LowHpActive && HEARTBEAT_PERIOD > 0.0f)
    {
        float phase = fmodf(m_HeartbeatTime, HEARTBEAT_PERIOD) / HEARTBEAT_PERIOD; // 0〜1
        float pulse = sinf(phase * 3.14159265f);
        if (pulse < 0.0f) pulse = 0.0f;
        pulse = pulse * pulse * pulse; // 立ち上がりを鋭く、他は静かに（ドクン……ドクン、のイメージ）
        heartbeatAlpha = LerpF(HEARTBEAT_MIN_ALPHA, HEARTBEAT_MAX_ALPHA, pulse);
    }

    return (hitAlpha > heartbeatAlpha) ? hitAlpha : heartbeatAlpha;
}
