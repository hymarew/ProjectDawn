#pragma once
#include <memory>
#include "vector3.h"
#include "weaponData.h"

class BulletPool;
class IAttackStrategy;

// =====================================================
// Weapon : 武器の実体（データ駆動の具象クラス）
// =====================================================
// 性能値は WeaponData（JSON マスタ）から、攻撃処理は IAttackStrategy から
// 受け取るため、武器ごとの派生クラスは作らない。
// 「武器 = WeaponData × AttackStrategy」の組み合わせで表現する。
// 生成は WeaponFactory::Create() が行う。
//
// このクラスの責務は残弾・連射間隔・リロードの状態管理のみ。
// Player からは TryFire() だけを呼ぶ。
// TryFire が連射間隔・残弾数・リロード中かどうかを確認し、
// 条件を満たしていれば AttackStrategy へ発射を委譲する。
class Weapon
{
public:
    // WeaponFactory から呼ばれる。data はコピーして保持する（Database の生存に依存しない）
    Weapon(const WeaponData& data, std::unique_ptr<IAttackStrategy> attackStrategy,
           BulletPool* pool);
    ~Weapon();

    // 毎フレーム呼ぶ。連射タイマーとリロードタイマーを進める
    void Update(float dt);

    // Player が攻撃入力時に呼ぶ。
    // リロード中・発射間隔未達・残弾0 の場合は発射しない
    void TryFire(const Vector3& origin, const Vector3& direction);

    // リロードを開始する（R キーなどから呼ぶ）
    void StartReload();

    // ---- UI・デバッグ用ゲッター ----
    const char*       GetName()   const { return m_Data.name.c_str(); }
    const WeaponData& GetData()   const { return m_Data; }
    WeaponID          GetID()     const { return m_Data.id; }
    Rarity            GetRarity() const { return m_Data.rarity; }

    int   GetFireCount()    const { return m_FireCount; }
    bool  IsSingleShot()    const { return m_Data.singleShot; }
    int   GetCurrentAmmo()  const { return m_CurrentAmmo; }
    int   GetMagazineSize() const { return m_Data.magazineSize; }
    bool  IsReloading()     const { return m_IsReloading; }

    // リロード進捗（0.0=開始直後, 1.0=完了）。リロード中以外は 1.0 を返す
    float GetReloadProgress() const
    {
        if (!m_IsReloading || m_Data.reloadTime <= 0.0f) return 1.0f;
        return 1.0f - (m_ReloadTimer / m_Data.reloadTime);
    }

private:
    WeaponData m_Data;  // 性能値（JSON マスタのコピー）

    std::unique_ptr<IAttackStrategy> m_AttackStrategy;  // 攻撃処理（発射時に委譲）
    BulletPool* m_BulletPool = nullptr;                 // 弾を Spawn するためのプール

    int   m_CurrentAmmo = 0;      // 現在の残弾数
    float m_FireTimer   = 999.0f; // 前回の発射からの経過時間（初期値を大きくして初弾を即発射可能にする）
    float m_ReloadTimer = 0.0f;   // リロードの残り時間（0になると完了）
    bool  m_IsReloading = false;  // リロード中かどうかのフラグ
    int   m_FireCount   = 0;      // トリガー操作回数（武器使用率集計用）
};
