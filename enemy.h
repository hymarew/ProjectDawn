#pragma once
#include "gameObject.h"
#include "renderer.h"
#include "vector3.h"
#include <vector>

// =====================================================
// Enemy : 全エネミー共通の親クラス
//
// HP・物理・ノックバック・TakeDamage など
// 全敵種で共通する処理をここに実装する。
// AI・モデル・シェーダは子クラスが担当する。
//
// 発見トリガー
//   ① TakeDamage  : 攻撃を受けたとき自分が起きる
//   ② UpdateAI    : プレイヤーが接近したとき（子クラスで実装）
//   ③ OnDamaged   : 攻撃を受けたとき周囲の仲間へ通知する
// =====================================================
class Enemy : public GameObject
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    virtual void Draw()       override {}
    virtual void DrawShadow() override {}

    virtual const char* GetName()     override { return "Enemy"; }
    virtual const char* GetTypeName() const    { return "Enemy"; }

    // ---- 生成 / 破棄 ----
    virtual void Spawn(const Vector3& pos, GameObject* target);
    virtual void Kill();

    // ① 攻撃を受けたとき：自分を起こし、③ 周囲へ通知する
    void TakeDamage(float dmg);
    void AddKnockback(const Vector3& dir, float power) { m_KnockbackVelocity += dir * power; }

    // 待機 → アクティブ への遷移（子クラスがオーバーライド）
    virtual void Alert() {}

    // ---- アクセサ ----
    float       GetHp()     const { return m_Hp; }
    bool        IsAlive()   const { return m_Hp > 0.0f; }
    GameObject* GetTarget() const { return m_Target; }
    void        SetTarget(GameObject* t) { m_Target = t; }

    // アクティブリスト / キル数（プール実装後はそちらへ移す）
    static std::vector<Enemy*>& GetActiveList() { return s_ActiveList; }
    static int  GetTotalKills()                  { return s_KillCount; }
    static void ResetKills()                     { s_KillCount = 0; }

protected:
    virtual void UpdateAI(float /*dt*/) {}

    // ③ 攻撃を受けたとき呼ばれる（子クラスが周囲への通知を実装）
    virtual void OnDamaged() {}

    GameObject* m_Target            = nullptr;
    float       m_Hp                = 0.0f;
    Vector3     m_Velocity          = {};
    Vector3     m_KnockbackVelocity = {};
    float       m_FallVelocityY     = 0.0f;
    LIGHT       Light               = {};

private:
    static std::vector<Enemy*> s_ActiveList;
    static int                 s_KillCount;
};
