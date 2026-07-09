#pragma once

// 2Dオフセット用の小さな構造体（HUDシェイクのみで使用）
struct Vector2
{
    float x = 0.0f;
    float y = 0.0f;
};

// =====================================================
// DamageEffectManager : プレイヤー被弾時のUI演出を一元管理する
//
// 方針（仕様書より）:
//   画面全体を赤くする・大きくカメラを揺らす、といった強い演出は
//   TPSのエイムを阻害するため採用しない。UI中心の演出に留める。
//
// 管理する演出:
//   - HUD全体の短いシェイク（0.15秒, ±4px, 減衰）
//   - HPゲージの赤フラッシュ（0.2秒で元の色へ線形補間）
//   - 画面端の赤ビネット（0.25秒, 0%→35%→0%）
//   - HP30%以下で心拍のようにゆっくり点滅するビネット（周期約1秒、HP回復で停止）
//
// 使い方:
//   毎フレーム   : g_DamageEffectManager.Update(dt, player->GetHp() / player->GetMaxHp());
//   被弾した瞬間 : g_DamageEffectManager.OnDamaged();
//   HUD描画時    : GetHudShakeOffset() / GetHpFlashRatio() / GetVignetteAlpha() を参照する
//
// カメラシェイクについて:
//   通常被弾では使わない。ロケット直撃や大爆発などの強い衝撃は
//   従来どおり各所で Camera::Shake() を直接呼ぶ（このクラスの管轄外）。
// =====================================================
class DamageEffectManager
{
public:
    void Update(float dt, float hpRatio);
    void OnDamaged();

    // HUD全体に加算するピクセルオフセット
    Vector2 GetHudShakeOffset() const { return m_ShakeOffset; }

    // 0(通常色)〜1(真っ赤)。HPバーの色を Lerp(通常色, 赤, これ) で求める
    float GetHpFlashRatio() const;

    // 赤ビネットの不透明度(0〜1)。被弾フラッシュと低HP心拍のうち大きい方を返す
    float GetVignetteAlpha() const;

private:
    // ---- HUDシェイク ----
    float   m_ShakeTimer  = 0.0f; // 残り時間
    Vector2 m_ShakeOffset;

    // ---- HPフラッシュ ----
    float m_HpFlashTimer = 0.0f; // 残り時間

    // ---- 被弾ビネット ----
    float m_HitVignetteTimer = 0.0f; // 残り時間

    // ---- 低HP心拍 ----
    float m_HeartbeatTime = 0.0f; // 経過時間（周期でループさせる位相用）
    bool  m_LowHpActive   = false;
};

extern DamageEffectManager g_DamageEffectManager;
