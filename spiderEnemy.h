#pragma once
#include "vector3.h"
#include <d3d11.h>
#include "gameObject.h"
#include "renderer.h"

enum class SpiderState
{
    Idle,    // 待機: プレイヤー未発見
    Chase,   // 追跡: 高速接近 + 分離
    Jump,    // 跳躍: ジャンプで距離を詰める
    Attack,  // 攻撃: 糸（スロー）/ 噛み付き / 張り付きストレイフ
    Search,  // 捜索: 最終目撃地点へ移動
};

class SpiderEnemy : public GameObject
{
public:
    void Init()   override;
    void Uninit() override;
    void Update(float dt) override;
    void Draw()   override;
    void DrawShadow() override;
    const char* GetName() override { return "SpiderEnemy"; }

    void Spawn(const Vector3& pos, GameObject* target);
    void Kill();
    void TakeDamage(float dmg);

    // 他の蜘蛛からの発見通知。Idle のときだけ Chase へ遷移する。
    void Alert();

    void AddKnockback(const Vector3& dir, float power) { m_KnockbackVelocity += dir * power; }

    float       GetHp()    const { return m_Hp; }
    SpiderState GetState() const { return m_State; }
    bool        IsAlive()  const { return m_Hp > 0.0f; }
    const char* GetTypeName() const { return "Spider"; }

private:
    static ID3D11InputLayout*  m_VertexLayout;
    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader*  m_PixelShader;
    static bool                m_IsLoaded;

    GameObject* m_Target = nullptr;

    LIGHT Light;

    float       m_Hp    = 120.0f;
    SpiderState m_State = SpiderState::Idle;

    Vector3 m_Velocity         = {};
    Vector3 m_KnockbackVelocity= {};
    float   m_FallVelocityY    = 0.0f;

    // ---- AI ステートフィールド ----
    Vector3 m_LastKnownPos  = {};     // Search で使う最終目撃地点
    float   m_WebCooldown   = 0.0f;  // 糸攻撃クールタイム（秒）
    float   m_SearchTimer   = 0.0f;  // Search ステートの残り時間（秒）
    float   m_JumpCooldown  = 0.0f;  // 連続ジャンプ防止クールタイム（秒）
    bool    m_IsGrounded    = true;  // 地面に接しているか（Jump 遷移条件）
    Vector3 m_JumpDir       = {};    // ジャンプ中の水平移動方向

    // 張り付きストレイフ
    float m_StrafeDir   = 1.0f;  // +1 = 右、-1 = 左
    float m_StrafeTimer = 0.0f;  // 次の方向転換までの残り時間（秒）

    // ---- ステート別 Update ----
    void UpdateIdle  (float dt);
    void UpdateChase (float dt);
    void UpdateJump  (float dt);
    void UpdateAttack(float dt);
    void UpdateSearch(float dt);

    // Chase 移動（追跡 + 分離を合成）
    void MoveChase(float dt);

    // 近傍蜘蛛との分離ベクトルを計算する
    Vector3 CalcSeparation() const;

    // 周囲の Idle 蜘蛛へ発見を通知する
    void AlertNearby();

    // ジャンプを開始する（Chase → Jump 遷移時に呼ぶ）
    void StartJump();
};
