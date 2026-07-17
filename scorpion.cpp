#include "main.h"
#include "scorpion.h"
#include "enemyProjectilePool.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "sphereCollider.h"
#include "particleManager.h"
#include "GameConfig.h"
#include <cstdlib>
#include <cmath>

ID3D11VertexShader* Scorpion::m_VertexShader = nullptr;
ID3D11PixelShader*  Scorpion::m_PixelShader  = nullptr;
ID3D11InputLayout*  Scorpion::m_VertexLayout = nullptr;
ID3D11Buffer*       Scorpion::m_FlashBuffer  = nullptr;
bool                Scorpion::m_IsLoaded     = false;

// FlashBuffer(b9) のCPU側ミラー。ScorpionPS.hlsl とレイアウトを一致させること
struct FlashCB
{
    float Intensity;
    float Dummy[3];
};

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
    if (m_FlashBuffer)  { m_FlashBuffer->Release();   m_FlashBuffer  = nullptr; }
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

        // ヒットフラッシュ用の定数バッファ（b9, 全個体共有。値は個体ごとに描画直前に書き込む）
        D3D11_BUFFER_DESC bd{};
        bd.Usage     = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(FlashCB);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_FlashBuffer);

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
    m_FlashIntensity   = 0.0f;
    m_FlashDecay       = 0.0f;
    m_DyingTimer       = 0.0f;

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
    // ヒットフラッシュの減衰（どの状態でも共通）
    if (m_FlashIntensity > 0.0f)
    {
        m_FlashIntensity -= dt * m_FlashDecay;
        if (m_FlashIntensity < 0.0f) m_FlashIntensity = 0.0f;
    }

    switch (m_State)
    {
    case EnemyState::Idle:    UpdateIdle    (dt); break;
    case EnemyState::Chase:   UpdateChase   (dt); break;
    case EnemyState::TailShot:UpdateTailShot(dt); break;
    case EnemyState::Dying:   UpdateDying   (dt); break;
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
// OnDamaged : 攻撃を受けたとき周囲の仲間を起こす（③）+ ヒットフラッシュ
// =====================================================
void Scorpion::OnDamaged()
{
    AlertNearby();

    // モデル全体を一瞬白く発光させる（硬い装甲に弾かれた衝撃の表現）
    m_FlashIntensity = 1.0f;
    m_FlashDecay     = GameConfig::ScorpionFX::FLASH_DECAY;
}

// =====================================================
// OnDeath : 即消滅せず死亡演出（Dying）へ遷移する
// 爆発は使わず「装甲が限界を迎えて砕け散る」イメージ。
// =====================================================
void Scorpion::OnDeath()
{
    m_State      = EnemyState::Dying;
    m_DyingTimer = GameConfig::ScorpionFX::DEATH_DURATION;
    m_Velocity   = {};

    // 通常より強く・長いフラッシュ
    m_FlashIntensity = 1.0f;
    m_FlashDecay     = GameConfig::ScorpionFX::DEATH_FLASH_DECAY;

    // 装甲崩壊エフェクト（火花大量・大きめの装甲片・粉・衝撃リング）
    Vector3 fxPos = GetPosition();
    fxPos.y += 1.0f; // 地面ギリギリではなく胴体の高さから飛び散らせる
    ParticleManager::GetInstance().EmitScorpionDeath(fxPos);
}

// =====================================================
// UpdateDying : 死亡演出中。フラッシュを見せてからモデルを消す
// =====================================================
void Scorpion::UpdateDying(float dt)
{
    m_Velocity = {};

    m_DyingTimer -= dt;
    if (m_DyingTimer <= 0.0f)
        Kill(); // エフェクト再生後にモデル消滅（アクティブリストから除外）
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
// GetHitSpheres : 被弾判定球（マルチスフィア）
//
// 「横に長い胴体＋高く持ち上がった尻尾」の体型を5個の球で覆う。
// オフセットは TAIL_OFFSET と同じくローカル空間のワールドスケール値
// （前方=+Z、尻尾は-Z側から上へアーチ状に伸びて先端が(0, 9.2, -5)）。
// 数値はコライダーデバッグ表示（g_ShowColliderDebug）で
// モデルに重ねて確認しながら調整すること。
// =====================================================
namespace
{
    constexpr HitSphere kScorpionHitSpheres[] = {
        { { 0.0f, 1.2f,  0.0f }, 2.2f }, // 胴体（低く幅広い本体）
        { { 0.0f, 1.4f,  2.4f }, 1.6f }, // 頭・ハサミ（前方）
        { { 0.0f, 3.2f, -3.6f }, 1.4f }, // 尻尾の付け根
        { { 0.0f, 6.2f, -4.6f }, 1.3f }, // 尻尾の中間
        { { 0.0f, 8.9f, -5.0f }, 1.4f }, // 尻尾の先端（毒針。TAIL_OFFSET付近）
    };
}

const HitSphere* Scorpion::GetHitSpheres(int& outCount) const
{
    outCount = static_cast<int>(sizeof(kScorpionHitSpheres) / sizeof(kScorpionHitSpheres[0]));
    return kScorpionHitSpheres;
}

float Scorpion::GetBroadPhaseRadius() const
{
    // 全HitSphereを包む半径（最遠の尻尾先端 |(0,8.9,-5)|≒10.2 + 半径1.4）
    return 12.0f;
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

    // 個体ごとのヒットフラッシュ強度をシェーダー(b9)へ書き込む
    FlashCB flash = { m_FlashIntensity, { 0.0f, 0.0f, 0.0f } };
    ctx->UpdateSubresource(m_FlashBuffer, 0, nullptr, &flash, 0, 0);
    ctx->PSSetConstantBuffers(9, 1, &m_FlashBuffer);

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
