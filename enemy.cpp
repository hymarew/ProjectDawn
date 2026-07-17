#include "main.h"
#include "enemy.h"
#include "manager.h"
#include "dropManager.h"
#include "GameConfig.h"
#include "playerLog.h"
#include <algorithm>
#include <cstdlib>

std::vector<Enemy*> Enemy::s_ActiveList;
int                 Enemy::s_KillCount = 0;

void Enemy::Init()
{
    m_Layer    = 1;
    m_IsActive = false; // プールで事前確保されるため、Spawn まで非アクティブ
    m_Position = { 0.0f, 0.0f, 0.0f };
    m_Scale    = { 1.0f, 1.0f, 1.0f };
}

void Enemy::Uninit()
{
    GameObject::Uninit();
}

// =====================================================
// Spawn : アクティブリストに登録して起こす
// startActive は基底クラスでは使用しない（子クラスがAI状態決定に使う）
// =====================================================
void Enemy::Spawn(const Vector3& pos, GameObject* target, bool /*startActive*/)
{
    m_IsActive          = true;
    m_Hp                = 0.0f;
    m_Velocity          = {};
    m_KnockbackVelocity = {};
    m_FallVelocityY     = 0.0f;
    m_Target            = target;
    SetPosition(pos);

    s_ActiveList.push_back(this);
}

// =====================================================
// Kill : アクティブリストから除外して眠らせる
// =====================================================
void Enemy::Kill()
{
    m_IsActive = false;
    s_ActiveList.erase(
        std::remove(s_ActiveList.begin(), s_ActiveList.end(), this),
        s_ActiveList.end());

    ++s_KillCount;
    g_PlayerLog.enemyKills[GetTypeName()]++;

    // ドロップ処理は DropManager へ委譲する。
    // 敵側は TypeName と座標を渡すだけで、テーブル・抽選方法を知らない。
    g_DropManager.OnEnemyKilled(GetTypeName(), m_Position);
}

// =====================================================
// Update : 共通物理処理 → AI の順に実行
// =====================================================
void Enemy::Update(float dt)
{
    if (!m_IsActive) return;
    GameObject::Update(dt);

    UpdateAI(dt);

    m_FallVelocityY -= GameConfig::Physics::GRAVITY * dt;
    m_Position.y    += m_FallVelocityY * dt;
    if (m_Position.y <= 0.0f)
    {
        m_Position.y    = 0.0f;
        m_FallVelocityY = 0.0f;
    }

    m_KnockbackVelocity -= m_KnockbackVelocity * GameConfig::RocketLauncher::KNOCKBACK_DECAY * dt;
    m_Position.x += m_Velocity.x + m_KnockbackVelocity.x * dt;
    m_Position.z += m_Velocity.z + m_KnockbackVelocity.z * dt;
}

// =====================================================
// TakeDamage
//   ① 自分を起こす（Idle → Chase）
//   ③ 周囲の仲間へ通知する
// =====================================================
void Enemy::TakeDamage(float dmg)
{
    // すでに死亡している（死亡演出中を含む）敵には二重にダメージを与えない
    if (m_Hp <= 0.0f) return;

    Alert();       // ① 自分が攻撃されたらアクティブになる
    m_Hp -= dmg;
    OnDamaged();   // ③ 周囲への通知（子クラスが実装）
    if (m_Hp <= 0.0f)
        OnDeath(); // デフォルトは即 Kill()。子クラスは死亡演出を挟める
}

// =====================================================
// GetHitSpheres : 被弾判定球の既定実装
// 子クラスが体型に合わせてオーバーライドする。
// 既定は「原点1個×SCORPION_RADIUS」＝従来の単一スフィア判定と同じ挙動
// =====================================================
const HitSphere* Enemy::GetHitSpheres(int& outCount) const
{
    static constexpr HitSphere kDefault[] = {
        { { 0.0f, 0.0f, 0.0f }, GameConfig::Collision::SCORPION_RADIUS },
    };
    outCount = 1;
    return kDefault;
}

float Enemy::GetBroadPhaseRadius() const
{
    return GameConfig::Collision::SCORPION_RADIUS;
}

// =====================================================
// TestHitSegment : 弾道の線分 vs HitSphere群の判定
//
// 各球について「線分上で球中心に最も近い点」を求め、その距離が
// (球半径+弾半径) 未満なら命中。複数の球に当たる場合は
// 弾道上で最も手前（fromに近い）の命中点を採用する。
// 線分で判定するため、1フレームに大きく進む高速弾でも
// 細い部位（尻尾等）をすり抜けない。
// =====================================================
bool Enemy::TestHitSegment(const Vector3& from, const Vector3& to,
                           float bulletRadius, Vector3& outHitPos) const
{
    int count = 0;
    const HitSphere* spheres = GetHitSpheres(count);

    const float   yaw     = m_Rotation.y;
    const Vector3 segment = to - from;
    const float   segLen2 = segment.LengthSq();

    bool  hit   = false;
    float bestT = 2.0f; // 線分上のパラメータ（0=from, 1=to）。最小のものを採用する

    for (int i = 0; i < count; i++)
    {
        // ローカルオフセットをYaw回転＋平行移動してワールド空間の球中心にする
        Vector3 center = m_Position + spheres[i].LocalOffset.RotatedAroundY(yaw);

        // 線分上で球中心に最も近い点のパラメータ t を求める（0〜1にクランプ）
        float t = 0.0f;
        if (segLen2 > 0.000001f)
        {
            Vector3 toCenter = center - from;
            t = (toCenter * segment) / segLen2; // 内積 ÷ 長さの2乗
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
        }

        Vector3 closest = from + segment * t;
        Vector3 diff    = center - closest;

        const float hitDist = spheres[i].Radius + bulletRadius;
        if (diff.LengthSq() < hitDist * hitDist && t < bestT)
        {
            hit       = true;
            bestT     = t;
            outHitPos = closest; // 最も手前の命中点（被弾エフェクトの発生位置に使う）
        }
    }

    return hit;
}
