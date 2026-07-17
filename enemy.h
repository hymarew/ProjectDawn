#pragma once
#include "gameObject.h"
#include "renderer.h"
#include "vector3.h"
#include <vector>

// =====================================================
// HitSphere : 弾の被弾判定用の球1個分の定義
//
// 敵1体を複数の小さな球の組み合わせ（マルチスフィア）で覆うことで、
// 「原点1点×大きな球1個」では当たらなかった尻尾・頭などの部位にも
// 弾が正しく当たるようにする。オフセットは敵のローカル空間
// （未回転・原点基準のワールドスケール値）で定義し、
// 判定時にYaw回転と位置を適用する。
// =====================================================
struct HitSphere
{
    Vector3 LocalOffset; // 敵原点からのオフセット（ローカル空間）
    float   Radius;      // 球の半径
};

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
    // startActive: true ならChase（Active）、false ならIdle（巡回）から開始する。
    // 実際のAI状態の切り替えは子クラス（Scorpion等）が担当する。
    virtual void Spawn(const Vector3& pos, GameObject* target, bool startActive = true);
    virtual void Kill();

    // ① 攻撃を受けたとき：自分を起こし、③ 周囲へ通知する
    void TakeDamage(float dmg);
    void AddKnockback(const Vector3& dir, float power) { m_KnockbackVelocity += dir * power; }

    // 待機 → アクティブ への遷移（子クラスがオーバーライド）
    virtual void Alert() {}

    // ---- 弾の被弾判定（マルチスフィア） ----

    // この敵の被弾判定球の配列を返す（子クラスが体型に合わせてオーバーライドする）。
    // 既定は「原点1個×SCORPION_RADIUS」で、従来の判定と同じ挙動になる
    virtual const HitSphere* GetHitSpheres(int& outCount) const;

    // 広域判定（バウンディング球）の半径。全HitSphereを包む大きさを子クラスが返す。
    // 弾の大半はこの1回の距離比較で棄却されるため、詳細判定の負荷はほぼ増えない
    virtual float GetBroadPhaseRadius() const;

    // 弾道の線分 [from→to] がこの敵のHitSphere群に当たるか調べる。
    // 点ではなく線分で判定することで、高速弾が細い部位（尻尾等）を
    // すり抜けるトンネリングを防ぐ。命中時は outHitPos に最も手前の命中点が入る
    bool TestHitSegment(const Vector3& from, const Vector3& to,
                        float bulletRadius, Vector3& outHitPos) const;

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

    // HPが0になったとき呼ばれる。デフォルトは即 Kill()。
    // 子クラスが死亡演出（フラッシュ・装甲崩壊エフェクト等）を挟みたい場合に
    // オーバーライドし、演出終了後に自分で Kill() を呼ぶ。
    virtual void OnDeath() { Kill(); }

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
