#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "spiderEnemy.h"
#include "spiderPool.h"
#include "sphereCollider.h"
#include "player.h"
#include "camera.h"
#include "GameConfig.h"
#include "playerLog.h"
#include <cstdlib>
#include <cmath>

ID3D11VertexShader* SpiderEnemy::m_VertexShader = nullptr;
ID3D11PixelShader*  SpiderEnemy::m_PixelShader  = nullptr;
ID3D11InputLayout*  SpiderEnemy::m_VertexLayout = nullptr;
bool SpiderEnemy::m_IsLoaded = false;

// =====================================================
// 内部ユーティリティ（XZ 平面での操作）
// =====================================================
static float SLen2D(const Vector3& v)
{
    return sqrtf(v.x * v.x + v.z * v.z);
}

static Vector3 SNorm2D(const Vector3& v)
{
    float len = SLen2D(v);
    if (len < 0.0001f) return { 0.0f, 0.0f, 0.0f };
    return { v.x / len, 0.0f, v.z / len };
}

// =====================================================
// Init / Uninit
// =====================================================
void SpiderEnemy::Init()
{
    m_Layer    = 1;
    m_Position = { 0.0f, 10.0f, 0.0f };

    // アリと同じモデルを暫定使用。専用モデルへの差し替えはここだけ変えればよい。
    AddComponent<ModelRenderer>(this)->Load("asset\\model\\enemy.obj");
    AddComponent<SphereCollider>(this)->Setup(
        GameConfig::Collision::SPIDER_RADIUS, ColliderTag::Enemy);

    if (!m_IsLoaded)
    {
        Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
            "shader\\ShadowMapLightingVS.cso");
        Renderer::CreatePixelShader(&m_PixelShader,
            "shader\\ShadowMapLightingPS.cso");
        m_IsLoaded = true;
    }
}

void SpiderEnemy::Uninit()
{
    GameObject::Uninit();
}

// =====================================================
// Spawn : プールから起こすときに呼ぶリセット処理
// =====================================================
void SpiderEnemy::Spawn(const Vector3& pos, GameObject* target)
{
    m_IsActive          = true;
    m_Hp                = GameConfig::Spider::HP;
    m_State             = SpiderState::Idle;
    m_WebCooldown       = 0.0f;
    m_SearchTimer       = 0.0f;
    m_JumpCooldown      = 0.0f;
    m_IsGrounded        = true;
    m_LastKnownPos      = {};
    m_JumpDir           = {};
    m_Velocity          = {};
    m_KnockbackVelocity = {};
    m_FallVelocityY     = 0.0f;
    m_StrafeDir         = (rand() % 2 == 0) ? 1.0f : -1.0f;
    m_StrafeTimer       = GameConfig::Spider::STRAFE_FLIP_TIME;

    SetPosition(pos);
    m_Target = target;
}

void SpiderEnemy::Kill()
{
    m_IsActive = false;
    g_PlayerLog.enemyKills[GetTypeName()]++;
}

void SpiderEnemy::TakeDamage(float dmg)
{
    m_Hp -= dmg;
    if (m_Hp <= 0.0f) Kill();
}

// =====================================================
// Alert : Idle 中の蜘蛛を Chase に遷移させる
// =====================================================
void SpiderEnemy::Alert()
{
    if (m_State == SpiderState::Idle)
        m_State = SpiderState::Chase;
}

// =====================================================
// AlertNearby : 周囲 ALERT_RADIUS 内の Idle 蜘蛛へ通知する
// =====================================================
void SpiderEnemy::AlertNearby()
{
    const float rSq = GameConfig::Spider::ALERT_RADIUS * GameConfig::Spider::ALERT_RADIUS;

    for (SpiderEnemy* other : g_SpiderPool.GetActiveSpiders())
    {
        if (other == this) continue;
        float dx = other->m_Position.x - m_Position.x;
        float dz = other->m_Position.z - m_Position.z;
        if (dx * dx + dz * dz <= rSq)
            other->Alert();
    }
}

// =====================================================
// CalcSeparation : 近傍蜘蛛との反発ベクトルを計算する
//
// 蜘蛛はアリより少ない分 SEPARATION_WEIGHT を高め（0.3）にして
// より広がりながら接近するEDF蜘蛛らしい動きにする。
// =====================================================
Vector3 SpiderEnemy::CalcSeparation() const
{
    Vector3 sep  = { 0.0f, 0.0f, 0.0f };
    int     cnt  = 0;
    const float rSq = GameConfig::Spider::SEPARATION_RADIUS
                    * GameConfig::Spider::SEPARATION_RADIUS;

    for (SpiderEnemy* other : g_SpiderPool.GetActiveSpiders())
    {
        if (other == this) continue;
        if (cnt >= GameConfig::Spider::MAX_SEP_CHECKS) break;

        float dx = m_Position.x - other->m_Position.x;
        float dz = m_Position.z - other->m_Position.z;
        float dSq = dx * dx + dz * dz;

        if (dSq < rSq && dSq > 0.0001f)
        {
            float d = sqrtf(dSq);
            sep.x  += (dx / d) * (GameConfig::Spider::SEPARATION_RADIUS - d);
            sep.z  += (dz / d) * (GameConfig::Spider::SEPARATION_RADIUS - d);
        }
        ++cnt;
    }

    return SNorm2D(sep);
}

// =====================================================
// MoveChase : 追跡 + 分離を合成した移動
// =====================================================
void SpiderEnemy::MoveChase(float dt)
{
    if (!m_Target) return;

    Vector3 chaseDir = SNorm2D(m_Target->GetPosition() - m_Position);
    Vector3 sepDir   = CalcSeparation();

    Vector3 finalDir;
    finalDir.x = chaseDir.x * GameConfig::Spider::CHASE_WEIGHT
               + sepDir.x   * GameConfig::Spider::SEPARATION_WEIGHT;
    finalDir.y = 0.0f;
    finalDir.z = chaseDir.z * GameConfig::Spider::CHASE_WEIGHT
               + sepDir.z   * GameConfig::Spider::SEPARATION_WEIGHT;
    finalDir = SNorm2D(finalDir);

    float speed  = GameConfig::Spider::MOVE_SPEED;
    m_Velocity.x = finalDir.x * speed * dt;
    m_Velocity.y = 0.0f;
    m_Velocity.z = finalDir.z * speed * dt;

    if (SLen2D(finalDir) > 0.01f)
        m_Rotation.y = atan2f(finalDir.x, finalDir.z);
}

// =====================================================
// StartJump : ジャンプ開始（Chase → Jump 遷移時）
// =====================================================
void SpiderEnemy::StartJump()
{
    if (!m_Target) return;

    m_JumpDir     = SNorm2D(m_Target->GetPosition() - m_Position);
    m_FallVelocityY = GameConfig::Spider::JUMP_POWER;
    m_IsGrounded  = false;
    m_State       = SpiderState::Jump;

    if (SLen2D(m_JumpDir) > 0.01f)
        m_Rotation.y = atan2f(m_JumpDir.x, m_JumpDir.z);
}

// =====================================================
// UpdateIdle : プレイヤーを検知したら Chase へ
// =====================================================
void SpiderEnemy::UpdateIdle(float dt)
{
    m_Velocity = {};
    if (!m_Target) return;

    float dist = SLen2D(m_Target->GetPosition() - m_Position);
    if (dist <= GameConfig::Spider::SENSE_RADIUS)
    {
        m_LastKnownPos = m_Target->GetPosition();
        m_State        = SpiderState::Chase;
        AlertNearby();
    }
}

// =====================================================
// UpdateChase : 高速追跡。条件を満たしたらジャンプ or 攻撃へ
// =====================================================
void SpiderEnemy::UpdateChase(float dt)
{
    if (!m_Target) { m_State = SpiderState::Search; return; }

    if (m_JumpCooldown > 0.0f) m_JumpCooldown -= dt;

    float dist = SLen2D(m_Target->GetPosition() - m_Position);

    // 攻撃範囲（糸 or 噛み付き）に入ったら Attack へ
    if (dist <= GameConfig::Spider::ATTACK_RANGE)
    {
        m_Velocity = {};
        m_State    = SpiderState::Attack;
        return;
    }

    // ジャンプ発動条件: 適切な距離 + 地面にいる + クールタイム切れ
    if (dist >= GameConfig::Spider::JUMP_MIN_DIST
     && dist <= GameConfig::Spider::JUMP_MAX_DIST
     && m_IsGrounded
     && m_JumpCooldown <= 0.0f)
    {
        StartJump();
        return;
    }

    // 見失い
    if (dist > GameConfig::Spider::LOSE_RADIUS)
    {
        m_SearchTimer = GameConfig::Spider::SEARCH_DURATION;
        m_State       = SpiderState::Search;
        return;
    }

    m_LastKnownPos = m_Target->GetPosition();
    MoveChase(dt);
}

// =====================================================
// UpdateJump : ジャンプ飛行中の更新
//
// 重力を適用しながら水平移動。着地で Chase に戻る。
// =====================================================
void SpiderEnemy::UpdateJump(float dt)
{
    // 水平移動（ジャンプ時の慣性）
    m_Velocity.x = m_JumpDir.x * GameConfig::Spider::JUMP_SPEED * dt;
    m_Velocity.y = 0.0f;
    m_Velocity.z = m_JumpDir.z * GameConfig::Spider::JUMP_SPEED * dt;

    m_FallVelocityY -= GameConfig::Physics::GRAVITY * dt;
    m_Position.y    += m_FallVelocityY * dt;

    // 着地判定
    if (m_Position.y <= 0.0f)
    {
        m_Position.y    = 0.0f;
        m_FallVelocityY = 0.0f;
        m_IsGrounded    = true;
        m_JumpCooldown  = GameConfig::Spider::JUMP_COOLDOWN;
        m_State         = SpiderState::Chase;
    }
}

// =====================================================
// UpdateAttack : 糸攻撃 / 噛み付き / 張り付きストレイフ
//
// 距離に応じて3つの行動を切り替える。
//   dist > ATTACK_RANGE : Chase に戻る
//   MELEE_RANGE < dist <= ATTACK_RANGE : 糸攻撃（スロー）
//   dist <= STRAFE_DIST : ストレイフしながら張り付く（噛み付きは衝突判定に任せる）
// =====================================================
void SpiderEnemy::UpdateAttack(float dt)
{
    m_Velocity = {};

    if (!m_Target) { m_State = SpiderState::Search; return; }

    float dist = SLen2D(m_Target->GetPosition() - m_Position);

    // 離れすぎたら Chase に戻る
    if (dist > GameConfig::Spider::ATTACK_RANGE * 1.5f)
    {
        m_State = SpiderState::Chase;
        return;
    }

    if (m_WebCooldown > 0.0f) m_WebCooldown -= dt;

    if (dist <= GameConfig::Spider::STRAFE_DIST)
    {
        // ---- 張り付きストレイフ ----
        // プレイヤーの周囲を横移動して狙いをつけさせない。
        // 実際の噛み付きダメージは SpiderPool vs Player の衝突判定で入る。
        m_StrafeTimer -= dt;
        if (m_StrafeTimer <= 0.0f)
        {
            m_StrafeDir   = -m_StrafeDir;
            m_StrafeTimer = GameConfig::Spider::STRAFE_FLIP_TIME;
        }

        Vector3 toTarget = SNorm2D(m_Target->GetPosition() - m_Position);
        Vector3 sideVec  = { -toTarget.z, 0.0f, toTarget.x };  // 90° 回転
        float   speed    = GameConfig::Spider::MOVE_SPEED * 0.5f;
        m_Velocity.x     = sideVec.x * m_StrafeDir * speed * dt;
        m_Velocity.y     = 0.0f;
        m_Velocity.z     = sideVec.z * m_StrafeDir * speed * dt;

        // プレイヤーを向き続ける
        m_Rotation.y = atan2f(toTarget.x, toTarget.z);
    }
    else if (dist <= GameConfig::Spider::ATTACK_RANGE)
    {
        // ---- 糸攻撃（スロー付与）----
        // 移動を止めてプレイヤーを向き、クールタイムが切れたらスローを付与する。
        Vector3 toTarget = SNorm2D(m_Target->GetPosition() - m_Position);
        m_Rotation.y = atan2f(toTarget.x, toTarget.z);

        if (m_WebCooldown <= 0.0f)
        {
            // m_Target が Player のときだけスローを適用する
            Player* p = dynamic_cast<Player*>(m_Target);
            if (p)
                p->ApplySlow(GameConfig::Spider::SLOW_DURATION,
                             GameConfig::Spider::SLOW_RATE);

            m_WebCooldown = GameConfig::Spider::ATTACK_COOLDOWN;
        }
    }
}

// =====================================================
// UpdateSearch : 最終目撃地点へ向かいタイムアウトで Idle へ
// =====================================================
void SpiderEnemy::UpdateSearch(float dt)
{
    m_SearchTimer -= dt;
    if (m_SearchTimer <= 0.0f)
    {
        m_Velocity = {};
        m_State    = SpiderState::Idle;
        return;
    }

    if (m_Target)
    {
        float dist = SLen2D(m_Target->GetPosition() - m_Position);
        if (dist <= GameConfig::Spider::SENSE_RADIUS)
        {
            m_LastKnownPos = m_Target->GetPosition();
            m_State        = SpiderState::Chase;
            return;
        }
    }

    Vector3 toGoal = m_LastKnownPos - m_Position;
    float   dist   = SLen2D(toGoal);
    if (dist < 1.0f)
    {
        m_Velocity = {};
        m_State    = SpiderState::Idle;
        return;
    }

    Vector3 dir  = SNorm2D(toGoal);
    float   speed = GameConfig::Spider::MOVE_SPEED * 0.6f;
    m_Velocity.x  = dir.x * speed * dt;
    m_Velocity.y  = 0.0f;
    m_Velocity.z  = dir.z * speed * dt;

    if (SLen2D(dir) > 0.01f)
        m_Rotation.y = atan2f(dir.x, dir.z);
}

// =====================================================
// Update : ステートマシンのディスパッチ + 物理更新
// =====================================================
void SpiderEnemy::Update(float dt)
{
    GameObject::Update(dt);

    switch (m_State)
    {
    case SpiderState::Idle:   UpdateIdle  (dt); break;
    case SpiderState::Chase:  UpdateChase (dt); break;
    case SpiderState::Jump:   UpdateJump  (dt); break;  // Jump は内部で重力も処理する
    case SpiderState::Attack: UpdateAttack(dt); break;
    case SpiderState::Search: UpdateSearch(dt); break;
    }

    // Jump ステートは UpdateJump 内で重力・Y移動を完結させる
    if (m_State != SpiderState::Jump)
    {
        m_FallVelocityY -= GameConfig::Physics::GRAVITY * dt;
        m_Position.y    += m_FallVelocityY * dt;

        if (m_Position.y <= 0.0f)
        {
            m_Position.y    = 0.0f;
            m_FallVelocityY = 0.0f;
            m_IsGrounded    = true;
        }
    }

    // ノックバック減衰
    m_KnockbackVelocity -= m_KnockbackVelocity * GameConfig::RocketLauncher::KNOCKBACK_DECAY * dt;

    // 水平移動（追跡 + ノックバック）
    m_Position.x += m_Velocity.x + m_KnockbackVelocity.x * dt;
    m_Position.z += m_Velocity.z + m_KnockbackVelocity.z * dt;
}

// =====================================================
// Draw / DrawShadow
// =====================================================
void SpiderEnemy::Draw()
{
    Camera* camera = Manager::GetCamera();
    if (!camera->CheckInView(m_Position)) return;

    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    Light.CastShadow = g_CastShadow;
    Renderer::SetLight(Light);

    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader,  NULL, 0);

    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling   (m_Scale.x,    m_Scale.y,    m_Scale.z);
    rot   = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);
    GameObject::Draw();
}

void SpiderEnemy::DrawShadow()
{
    Camera* camera = Manager::GetCamera();
    if (!camera->CheckInView(m_Position, 3.0f)) return;

    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling   (m_Scale.x,    m_Scale.y,    m_Scale.z);
    rot   = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);
    GameObject::DrawShadow();
}
