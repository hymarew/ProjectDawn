#pragma once
#include "vector3.h"
#include <d3d11.h>
#include <vector>
#include "gameObject.h"
#include "renderer.h"
#include "GameConfig.h"

class Weapon;
class SphereCollider;

class Player :public GameObject
{
private:
    Vector3 m_Velocity{ 0.0f,0.0f,0.0f };

    ID3D11InputLayout* m_VertexLayout;
    ID3D11VertexShader* m_VertexShader;
    ID3D11PixelShader* m_PixelShader;

    LIGHT Light;
    float LightMoveSpeed;
    bool m_Ground = true;
    float m_MoveAnimation;

    // 所持している武器の一覧。Init で全武器を生成して登録する
    std::vector<Weapon*> m_Weapons;

    // 現在装備中の武器のインデックス（m_Weapons の添え字）
    int m_WeaponIndex = 0;

    // HP と無敵時間
    float m_Hp      = 100.0f;
    float m_InvTimer = 0.0f;  // 0 より大きい間は被弾しない

    // スロー効果（蜘蛛の糸攻撃などで付与される移動速度低下）
    float m_SlowTimer = 0.0f;  // 残り持続時間（秒）。0 以下 = 効果なし
    float m_SlowMult  = 1.0f;  // 速度倍率（< 1.0f のとき遅くなる）

public:
    void Init()override;
    void Uninit()override;
    void Update(float dt) override;
    void Draw()override;
    void DrawShadow()override;
    const char* GetName() override { return "Player"; }

    // 現在の武器を返す。外部から射撃や状態表示に使う
    Weapon* GetWeapon() const;

    // ImGui などから武器インデックスを直接指定して切り替える
    void SetWeaponIndex(int index);

    int GetWeaponIndex() const             { return m_WeaponIndex; }
    int GetWeaponCount() const             { return (int)m_Weapons.size(); }
    std::vector<Weapon*>& GetWeapons()     { return m_Weapons; }

    // source : 攻撃した敵タイプ名（"Alien" など）。ログ記録に使用
    void  TakeDamage(float dmg, const char* source = "Alien");

    // HP回復（最大HPを超えないようClampする）。回復アイテム等から呼ばれる
    void  Heal(float amount)
    {
        m_Hp += amount;
        if (m_Hp > GetMaxHp()) m_Hp = GetMaxHp();
    }

    float GetHp()    const { return m_Hp; }
    float GetMaxHp() const { return GameConfig::Player::MAX_HP; }
    bool  IsAlive()  const { return m_Hp > 0.0f; }

    // BoxCollider の上面に着地したとき CollisionManager から呼ばれる
    void ZeroFallVelocity() { if (m_Velocity.y < 0.0f) m_Velocity.y = 0.0f; }

    // スロー効果を付与する。より長い duration が残っているときは更新しない
    void  ApplySlow(float duration, float mult);
    float GetSpeedMult() const { return (m_SlowTimer > 0.0f) ? m_SlowMult : 1.0f; }
    bool  IsSlowed()     const { return m_SlowTimer > 0.0f; }
};