#pragma once
#include "enemy.h"
#include <d3d11.h>

enum class EnemyState { Idle, Chase, TailShot, Dying };

// =====================================================
// Scorpion : サソリ型エネミー
//
// Enemy を継承し、AI（Idle / Chase / TailShot / Dying）・
// モデル・シェーダを実装する。
//
// エフェクト:
//   被弾時はモデル全体が一瞬白く光る（ヒットフラッシュ, FlashBuffer:b9）。
//   撃破時は即消滅せず Dying 状態に入り、強めのフラッシュと
//   装甲崩壊エフェクトを再生してからモデルを消す（爆発はしない）。
// =====================================================
class Scorpion : public Enemy
{
public:
    void Init()           override;
    void Uninit()         override;
    void Spawn(const Vector3& pos, GameObject* target, bool startActive = true) override;
    void Draw()           override;
    void DrawShadow()     override;
    void Alert()          override;

    const char* GetName()     override { return "Scorpion"; }
    const char* GetTypeName() const override { return "Scorpion"; }

    static void ReleaseShaders();

protected:
    void UpdateAI  (float dt) override;
    void OnDamaged ()         override; // ③ 周囲の仲間へ発見通知 + ヒットフラッシュ
    void OnDeath   ()         override; // 死亡演出（Dying状態）へ遷移する

private:
    EnemyState m_State            = EnemyState::Idle;
    float      m_TailShotCooldown = 0.0f;
    float      m_TailShotWindup   = 0.0f;
    bool       m_TailShotReady    = false;
    float      m_SurroundOffset   = 0.0f;

    // ---- ヒットフラッシュ / 死亡演出 ----
    float      m_FlashIntensity   = 0.0f; // 0=通常 〜 1=真っ白（毎フレーム減衰）
    float      m_FlashDecay       = 0.0f; // 減衰速度（通常ヒットと撃破時で異なる）
    float      m_DyingTimer       = 0.0f; // Dying状態の残り時間（0でモデル消滅）

    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader*  m_PixelShader;
    static ID3D11InputLayout*  m_VertexLayout;
    static ID3D11Buffer*       m_FlashBuffer; // ヒットフラッシュ用cbuffer（b9, 全個体共有）
    static bool                m_IsLoaded;

    // モデル空間での尻尾先端オフセット（全個体共通）
    static constexpr Vector3 TAIL_OFFSET = { 0.0f, 9.2f, -5.0f };

    void UpdateIdle    (float dt);
    void UpdateChase   (float dt);
    void UpdateTailShot(float dt);
    void UpdateDying   (float dt);

    void    MoveChase       (float dt);
    Vector3 CalcSeparation  () const;
    void    AlertNearby     ();
    Vector3 GetTailPosition () const;
};
