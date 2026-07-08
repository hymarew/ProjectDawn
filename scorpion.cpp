#include "main.h"
#include "scorpion.h"
#include "enemyProjectilePool.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "sphereCollider.h"
#include "GameConfig.h"
#include <cstdlib>
#include <cmath>

ID3D11VertexShader* Scorpion::m_VertexShader = nullptr;
ID3D11PixelShader*  Scorpion::m_PixelShader  = nullptr;
ID3D11InputLayout*  Scorpion::m_VertexLayout = nullptr;
bool                Scorpion::m_IsLoaded     = false;

// ---- XZ 平面ユーティリティ ----
static float SLen2D(const Vector3& v) { return sqrtf(v.x * v.x + v.z * v.z); }
static Vector3 SNorm2D(const Vector3& v)
{
    float len = SLen2D(v);
    if (len < 0.0001f) return {};
    return { v.x / len, 0.0f, v.z / len };
}

// =====================================================
// ReleaseShaders : アプリ終了時に呼ぶ
// =====================================================
void Scorpion::ReleaseShaders()
{
    if (m_VertexShader) { m_VertexShader->Release(); m_VertexShader = nullptr; }
    if (m_PixelShader)  { m_PixelShader->Release();  m_PixelShader  = nullptr; }
    if (m_VertexLayout) { m_VertexLayout->Release();  m_VertexLayout = nullptr; }
    m_IsLoaded = false;
}

// =====================================================
// Init : モデル・コライダー・シェーダの初期化
// =====================================================
void Scorpion::Init()
{
    Enemy::Init();
    m_Scale = { 1.5f, 1.5f, 1.5f };

    AddComponent<ModelRenderer>(this)->Load("asset\\model\\Scorpion.obj");
    AddComponent<SphereCollider>(this)->Setup(
        GameConfig::Collision::SCORPION_RADIUS, ColliderTag::Enemy);

    if (!m_IsLoaded)
    {
        Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
            "shader\\ShadowMapLightingVS.cso");
        Renderer::CreatePixelShader(&m_PixelShader, "shader\\ScorpionPS.cso");
        m_IsLoaded = true;
    }

    XMVECTOR dir = XMVector3Normalize(XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f));
    XMStoreFloat4(&Light.Direction, dir);
    Light.Enable  = TRUE;
    Light.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    Light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
}

void Scorpion::Uninit()
{
    Enemy::Uninit();
}

// =====================================================
// Spawn : AI 状態をリセットして起こす
// startActive が false のときは Idle（巡回）から開始し、
// プレイヤーを索敵するまで Chase へ遷移しない。
// =====================================================
void Scorpion::Spawn(const Vector3& pos, GameObject* target, bool startActive)
{
    Enemy::Spawn(pos, target, startActive);
    m_Hp               = GameConfig::Scorpion::HP;
    m_State            = startActive ? EnemyState::Chase : EnemyState::Idle;
    m_TailShotCooldown = 0.0f;
    m_TailShotWindup   = 0.0f;
    m_TailShotReady    = false;

    float range      = GameConfig::Scorpion::SURROUND_OFFSET;
    m_SurroundOffset = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * range;
}

// =====================================================
// Alert : Idle 中のみ Chase へ
// =====================================================
void Scorpion::Alert()
{
    if (m_State == EnemyState::Idle)
        m_State = EnemyState::Chase;
}

// =====================================================
// UpdateAI : ステートディスパッチ
// =====================================================
void Scorpion::UpdateAI(float dt)
{
    switch (m_State)
    {
    case EnemyState::Idle:    UpdateIdle    (dt); break;
    case EnemyState::Chase:   UpdateChase   (dt); break;
    case EnemyState::TailShot:UpdateTailShot(dt); break;
    }
}

// =====================================================
// AlertNearby : 周囲の Idle エネミーへ発見を通知する
// ※ Enemy::s_ActiveList を使用。プール実装後はそちらへ移す
// =====================================================
void Scorpion::AlertNearby()
{
    const float r2 = GameConfig::Scorpion::ALERT_RADIUS * GameConfig::Scorpion::ALERT_RADIUS;
    for (Enemy* e : Enemy::GetActiveList())
    {
        if (e == this || !e->IsAlive()) continue;
        float dx = e->GetPosition().x - m_Position.x;
        float dz = e->GetPosition().z - m_Position.z;
        if (dx * dx + dz * dz <= r2)
            e->Alert();
    }
}

// =====================================================
// OnDamaged : 攻撃を受けたとき周囲の仲間を起こす（③）
// =====================================================
void Scorpion::OnDamaged()
{
    AlertNearby();
}

// =====================================================
// CalcSeparation : 近傍との反発ベクトル
// TODO: プール実装後に追加する
// =====================================================
Vector3 Scorpion::CalcSeparation() const
{
    // プールが未実装のため現在はスタブ
    return {};
}

// =====================================================
// MoveChase : 追跡 + 分離 + 囲み補正
// =====================================================
void Scorpion::MoveChase(float dt)
{
    if (!m_Target) return;

    Vector3 toTarget = m_Target->GetPosition() - m_Position;
    float   dist     = SLen2D(toTarget);

    Vector3 targetPos = m_Target->GetPosition();
    if (dist < GameConfig::Scorpion::SURROUND_DIST)
    {
        Vector3 forward = SNorm2D(toTarget);
        Vector3 sideVec = { -forward.z, 0.0f, forward.x };
        targetPos.x    += sideVec.x * m_SurroundOffset;
        targetPos.z    += sideVec.z * m_SurroundOffset;
    }

    Vector3 chaseDir = SNorm2D(targetPos - m_Position);
    Vector3 sepDir   = CalcSeparation();

    Vector3 finalDir;
    finalDir.x = chaseDir.x * GameConfig::Scorpion::CHASE_WEIGHT
               + sepDir.x   * GameConfig::Scorpion::SEPARATION_WEIGHT;
    finalDir.y = 0.0f;
    finalDir.z = chaseDir.z * GameConfig::Scorpion::CHASE_WEIGHT
               + sepDir.z   * GameConfig::Scorpion::SEPARATION_WEIGHT;
    finalDir = SNorm2D(finalDir);

    float speed = GameConfig::Scorpion::MOVE_SPEED;
    m_Velocity.x = finalDir.x * speed * dt;
    m_Velocity.y = 0.0f;
    m_Velocity.z = finalDir.z * speed * dt;

    if (SLen2D(finalDir) > 0.01f)
    {
        Vector3 rot = GetRotation();
        rot.y = atan2f(finalDir.x, finalDir.z);
        SetRotation(rot);
    }
}

// =====================================================
// GetTailPosition : 尻尾先端のワールド座標
// =====================================================
Vector3 Scorpion::GetTailPosition() const
{
    return GetPosition() + TAIL_OFFSET.RotatedAroundY(GetRotation().y);
}

// =====================================================
// UpdateIdle : 索敵 → Chase
// =====================================================
void Scorpion::UpdateIdle(float dt)
{
    m_Velocity = {};
    if (!m_Target) return;

    float dist = SLen2D(m_Target->GetPosition() - m_Position);
    if (dist <= GameConfig::Scorpion::SENSE_RADIUS)
    {
        m_State = EnemyState::Chase;
        AlertNearby();
    }
}

// =====================================================
// UpdateChase : 追跡 / TailShot 遷移 / 見失い
// =====================================================
void Scorpion::UpdateChase(float dt)
{
    if (!m_Target) { m_State = EnemyState::Idle; return; }

    if (m_TailShotCooldown > 0.0f) m_TailShotCooldown -= dt;

    float dist = SLen2D(m_Target->GetPosition() - m_Position);

    if (dist >= GameConfig::ScorpionNeedle::RANGE_MIN
     && dist <= GameConfig::ScorpionNeedle::RANGE_MAX
     && m_TailShotCooldown <= 0.0f)
    {
        m_Velocity         = {};
        m_TailShotWindup   = GameConfig::ScorpionNeedle::WINDUP;
        m_TailShotReady    = false;
        m_State            = EnemyState::TailShot;
        return;
    }

    if (dist > GameConfig::Scorpion::LOSE_RADIUS)
    {
        m_State = EnemyState::Idle;
        return;
    }

    MoveChase(dt);
}

// =====================================================
// UpdateTailShot : 予兆 → 発射 → Chase へ
// =====================================================
void Scorpion::UpdateTailShot(float dt)
{
    m_Velocity = {};
    if (!m_Target) { m_State = EnemyState::Chase; return; }

    Vector3 toTarget = SNorm2D(m_Target->GetPosition() - m_Position);
    if (SLen2D(toTarget) > 0.01f)
    {
        Vector3 rot = GetRotation();
        rot.y = atan2f(toTarget.x, toTarget.z);
        SetRotation(rot);
    }

    m_TailShotWindup -= dt;

    if (m_TailShotWindup <= 0.0f && !m_TailShotReady)
    {
        Vector3 tip   = GetTailPosition();
        Vector3 dir3D = m_Target->GetPosition() - tip;
        float   len   = dir3D.Length();
        if (len > 0.01f) dir3D = dir3D / len;
        else              dir3D = { toTarget.x, 0.0f, toTarget.z };

        constexpr float spread = GameConfig::EnemyProjectile::NEEDLE_SPREAD_ANGLE;
        constexpr float offsets[GameConfig::EnemyProjectile::NEEDLE_COUNT] =
            { -spread, 0.0f, spread };

        for (int ni = 0; ni < GameConfig::EnemyProjectile::NEEDLE_COUNT; ++ni)
        {
            float cosA = cosf(offsets[ni]);
            float sinA = sinf(offsets[ni]);
            Vector3 d = {
                dir3D.x * cosA + dir3D.z * sinA,
                dir3D.y,
               -dir3D.x * sinA + dir3D.z * cosA
            };
            g_EnemyProjectilePool.Spawn(
                EnemyProjectileType::Needle,
                tip,
                d,
                GameConfig::EnemyProjectile::NEEDLE_SPEED,
                GameConfig::EnemyProjectile::NEEDLE_DAMAGE);
        }

        m_TailShotCooldown = GameConfig::ScorpionNeedle::FIRE_INTERVAL;
        m_TailShotReady    = true;
        m_State            = EnemyState::Chase;
    }
}

// =====================================================
// Draw / DrawShadow
// =====================================================
void Scorpion::Draw()
{
    auto* ctx = Renderer::GetDeviceContext();
    ctx->IASetInputLayout(m_VertexLayout);
    Light.CastShadow = g_CastShadow;
    Renderer::SetLight(Light);
    ctx->VSSetShader(m_VertexShader, nullptr, 0);
    ctx->PSSetShader(m_PixelShader,  nullptr, 0);

    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z)
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    Renderer::SetWorldMatrix(world);
    GameObject::Draw();
}

void Scorpion::DrawShadow()
{
    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z)
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    Renderer::SetWorldMatrix(world);
    GameObject::DrawShadow();
}
