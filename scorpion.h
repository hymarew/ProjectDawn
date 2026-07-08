#pragma once
#include "enemy.h"
#include <d3d11.h>

enum class EnemyState { Idle, Chase, TailShot };

// =====================================================
// Scorpion : サソリ型エネミー
//
// Enemy を継承し、AI（Idle / Chase / TailShot）・
// モデル・シェーダを実装する。
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
    void OnDamaged ()         override; // ③ 周囲の仲間へ発見通知

private:
    EnemyState m_State            = EnemyState::Idle;
    float      m_TailShotCooldown = 0.0f;
    float      m_TailShotWindup   = 0.0f;
    bool       m_TailShotReady    = false;
    float      m_SurroundOffset   = 0.0f;

    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader*  m_PixelShader;
    static ID3D11InputLayout*  m_VertexLayout;
    static bool                m_IsLoaded;

    // モデル空間での尻尾先端オフセット（全個体共通）
    static constexpr Vector3 TAIL_OFFSET = { 0.0f, 9.2f, -5.0f };

    void UpdateIdle    (float dt);
    void UpdateChase   (float dt);
    void UpdateTailShot(float dt);

    void    MoveChase       (float dt);
    Vector3 CalcSeparation  () const;
    void    AlertNearby     ();
    Vector3 GetTailPosition () const;
};
