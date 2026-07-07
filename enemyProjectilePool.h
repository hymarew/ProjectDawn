#pragma once
#include "enemyProjectile.h"
#include <vector>
#include <d3d11.h>

class Player;

// =====================================================
// EnemyProjectilePool : 全敵弾を一元管理するプール
//
// - 弾の生成・更新・削除・プレイヤーとの当たり判定
// - Behavior は Spawn() を呼ぶだけでよい
// - 弾種追加は EnemyProjectileType の追加のみ
// =====================================================
class EnemyProjectilePool
{
public:
    void Init();
    void Uninit();
    void Update(float dt, Player* player);
    void Draw();

    // type と speed/damage は呼び出し側（Behavior）が GameConfig から渡す
    // radius / lifeTime はプール内部が type に応じて決定する
    void Spawn(
        EnemyProjectileType type,
        const Vector3&      position,
        const Vector3&      direction,
        float               speed,
        float               damage);

    int GetActiveCount() const;

private:
    std::vector<EnemyProjectile> m_Pool;

    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader*  m_PixelShader;
    static ID3D11InputLayout*  m_VertexLayout;
    static bool                m_IsLoaded;

    // 弾種ごとの内部パラメータを返す
    static float GetRadius  (EnemyProjectileType type);
    static float GetLifeTime(EnemyProjectileType type);
    static float GetScale   (EnemyProjectileType type);
};

extern EnemyProjectilePool g_EnemyProjectilePool;
