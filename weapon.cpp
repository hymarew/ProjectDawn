#include "weapon.h"
#include "bulletPool.h"

// -------------------------------------------------------
// Update : 毎フレームの時間管理
// -------------------------------------------------------
// 連射タイマーを進め、リロード中であればカウントダウンする。
// Player::Update から毎フレーム呼ばれることを前提としている。
void Weapon::Update(float dt)
{
    // 発射間隔タイマーを進める（TryFire でリセットされる）
    m_FireTimer += dt;

    // リロード中のカウントダウン処理
    if (m_IsReloading)
    {
        m_ReloadTimer -= dt;

        // タイマーが 0 以下になったらリロード完了
        if (m_ReloadTimer <= 0.0f)
        {
            m_CurrentAmmo = m_WeaponParams.magazineSize; // 弾倉を満タンにする
            m_IsReloading = false;
        }
    }
}

// -------------------------------------------------------
// TryFire : 発射条件チェックをしてから Fire() を呼ぶ
// -------------------------------------------------------
// Player は発射条件を知る必要がない。
// 「攻撃ボタンが押された」ときにこれを呼ぶだけでよい。
void Weapon::TryFire(const Vector3& origin, const Vector3& direction)
{
    // リロード中は発射できない
    if (m_IsReloading) return;

    // 前回の発射から発射間隔（fireRate）が経過していなければ発射できない
    if (m_FireTimer < m_WeaponParams.fireRate) return;

    // magazineSize == -1 は弾数無制限なので残弾チェックをスキップ
    if (m_WeaponParams.magazineSize != -1)
    {
        if (m_CurrentAmmo <= 0)
        {
            // 残弾0で撃とうとしたら自動リロード開始
            StartReload();
            return;
        }
        m_CurrentAmmo--; // 残弾を1減らす
    }

    // 発射タイマーをリセットして次の発射間隔を計測し始める
    m_FireTimer = 0.0f;

    m_FireCount++;

    // 派生クラスの発射処理（弾を BulletPool に Spawn する）を呼ぶ
    Fire(origin, direction);

    // 発射後に残弾が0になったら自動でリロードを開始する
    if (m_WeaponParams.magazineSize != -1 && m_CurrentAmmo <= 0)
        StartReload();
}

// -------------------------------------------------------
// StartReload : リロード開始
// -------------------------------------------------------
// 以下の場合はリロードしない：
//   - すでにリロード中
//   - 弾数無制限（magazineSize == -1）
//   - すでに満タン
void Weapon::StartReload()
{
    if (m_IsReloading) return;
    if (m_WeaponParams.magazineSize == -1) return;
    if (m_CurrentAmmo == m_WeaponParams.magazineSize) return;

    m_IsReloading = true;
    m_ReloadTimer = m_WeaponParams.reloadTime; // リロードタイマーをセット
}
