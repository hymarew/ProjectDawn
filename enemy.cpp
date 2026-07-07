#include "main.h"
#include "enemy.h"
#include "manager.h"
#include "GameConfig.h"
#include "playerLog.h"
#include <algorithm>

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
// =====================================================
void Enemy::Spawn(const Vector3& pos, GameObject* target)
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
    Alert();       // ① 自分が攻撃されたらアクティブになる
    m_Hp -= dmg;
    OnDamaged();   // ③ 周囲への通知（子クラスが実装）
    if (m_Hp <= 0.0f)
        Kill();
}
