#include "pistol.h"
#include "bulletPool.h"

// -------------------------------------------------------
// Init : ピストルのパラメータ設定
// -------------------------------------------------------
// 武器パラメータ（連射速度・弾倉・リロード時間）と
// 弾パラメータ（速度・ダメージ・寿命）をここで決める。
// 数値を変えるだけで性能調整ができる。
void Pistol::Init(BulletPool* pool)
{
    // BulletPool のポインタを基底クラスのメンバに保存する（Fire で使う）
    m_BulletPool = pool;

    // ---- 武器挙動パラメータの設定 ----
    m_WeaponParams.magazineSize = 12;    // 1マガジン12発
    m_WeaponParams.fireRate     = 0.25f; // 0.25秒ごとに1発（最大4連射/秒）
    m_WeaponParams.reloadTime   = 1.2f;  // リロードに1.2秒かかる
    m_WeaponParams.zoomRatio    = 1.0f;  // ADSズームなし

    // ---- 弾の性能パラメータの設定 ----
    m_BulletParams.damage        = 10.0f; // 1発10ダメージ
    m_BulletParams.speed         = 50.0f; // 弾速50（単位/秒）
    m_BulletParams.lifeTime      = 2.0f;  // 2秒で消滅（= 最大射程100）
    m_BulletParams.spreadAngle   = 0.0f;  // 拡散なし（完全直線）
    m_BulletParams.bulletPerShot = 1;     // 1発射で弾1発

    // 初期弾数をマガジン満タンにする
    m_CurrentAmmo = m_WeaponParams.magazineSize;
}

// -------------------------------------------------------
// Fire : 弾を1発 BulletPool に Spawn する
// -------------------------------------------------------
// TryFire から「発射OK」と判断されたときだけ呼ばれる。
// direction に弾速を掛けて velocity（速度ベクトル）を作り、
// BulletPool に Spawn を依頼する。
void Pistol::Fire(const Vector3& origin, const Vector3& direction)
{
    // direction は正規化済みの向きベクトル。
    // speed を掛けることで「その方向にどれだけ速く飛ぶか」を表す velocity になる
    Vector3 vel = direction * m_BulletParams.speed;

    // BulletPool の空きスロットを借りて弾を出現させる
    m_BulletPool->Spawn(origin, vel, m_BulletParams.lifeTime, m_BulletParams.damage);
}
