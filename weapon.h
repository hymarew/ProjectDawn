#pragma once
#include "vector3.h"
#include "bulletParams.h"
#include "weaponParams.h"

class BulletPool;

// =====================================================
// Weapon : 全武器の基底クラス
// =====================================================
// Pistol・Shotgun・MachineGun など、すべての武器はこのクラスを継承する。
//
// 連射間隔・残弾数・リロードの管理はこの基底クラスが行うため、
// 派生クラスは「1発撃ったときに何をするか（Fire）」だけを実装すればよい。
//
// Player からは TryFire() だけを呼ぶ。
// TryFire が連射間隔・残弾数・リロード中かどうかを確認し、
// 条件を満たしていれば Fire() を呼ぶ。
class Weapon
{
protected:
    WeaponParams m_WeaponParams = {}; // 連射速度・弾倉・リロード時間などの武器挙動
    BulletParams m_BulletParams = {}; // 弾速・ダメージ・拡散角などの弾性能

    int   m_CurrentAmmo = 0;      // 現在の残弾数
    float m_FireTimer   = 999.0f; // 前回の発射からの経過時間（初期値を大きくして初弾を即発射可能にする）
    float m_ReloadTimer = 0.0f;   // リロードの残り時間（0になると完了）
    bool  m_IsReloading = false;  // リロード中かどうかのフラグ
    int   m_FireCount   = 0;      // トリガー操作回数（武器使用率集計用）

    BulletPool* m_BulletPool = nullptr; // 弾を Spawn するためのプール（外部から渡される）

    // 派生クラスが実装する「実際に弾を発射する処理」
    // TryFire から条件チェック後に呼ばれる
    virtual void Fire(const Vector3& origin, const Vector3& direction) = 0;

public:
    virtual ~Weapon() = default;

    // 弾プールを受け取り、武器パラメータを設定する
    virtual void Init(BulletPool* pool) = 0;

    virtual void Uninit() = 0;

    // 毎フレーム呼ぶ。連射タイマーとリロードタイマーを進める
    virtual void Update(float dt);

    // Player が攻撃入力時に呼ぶ。
    // リロード中・発射間隔未達・残弾0 の場合は発射しない
    void TryFire(const Vector3& origin, const Vector3& direction);

    // リロードを開始する（R キーなどから呼ぶ）
    void StartReload();

    virtual const char* GetName() const = 0;

    // UI・デバッグ用ゲッター
    int   GetFireCount()        const { return m_FireCount; }
    bool  IsSingleShot()       const { return m_WeaponParams.singleShot; }
    int   GetCurrentAmmo()     const { return m_CurrentAmmo; }
    int   GetMagazineSize()    const { return m_WeaponParams.magazineSize; }
    bool  IsReloading()        const { return m_IsReloading; }

    // リロード進捗（0.0=開始直後, 1.0=完了）。リロード中以外は 1.0 を返す
    float GetReloadProgress()  const
    {
        if (!m_IsReloading || m_WeaponParams.reloadTime <= 0.0f) return 1.0f;
        return 1.0f - (m_ReloadTimer / m_WeaponParams.reloadTime);
    }
};
