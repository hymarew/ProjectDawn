// ===================================================
// particleEmitter.cpp
// 1回分のエフェクトを管理するエミッタクラス
//
// 【役割】
//   ParticleSetting（エフェクトの設定データ）を受け取り、
//   毎フレーム SpawnRate に従ってパーティクルを放出する。
//   自分ではパーティクルの配列を持たず、
//   ParticleManager が管理するグローバルプールに書き込む。
// ===================================================

#include "particleEmitter.h"

// ---------------------------------------------------------
// Init : エミッタの初期化
// ---------------------------------------------------------
void ParticleEmitter::Init(const ParticleSetting& setting, Vector3 position)
{
    m_Setting    = setting;           // エフェクトの設定データ（速度・寿命・色など）
    m_Position   = position;          // パーティクルを放出する位置
    m_Life       = setting.EmitterLife; // エミッタ自身の寿命（秒）
    m_SpawnTimer = 0.0f;              // 最初はすぐに放出できるようゼロにしておく

    // エミッタごとに独立した乱数エンジンを作る
    // （複数エミッタが同時に動いても、それぞれ別の乱数列を使う）
    std::random_device rd;
    m_Rng = std::mt19937(rd());
}

// ---------------------------------------------------------
// Update : 放出タイミングの管理とパーティクルの初期化
// ---------------------------------------------------------
void ParticleEmitter::Update(float dt, ParticleData* pool, int poolSize, int& nextFree)
{
    // ---- エミッタの寿命を減らす ----
    m_Life -= dt;

    // 寿命が尽きたら何もしない（IsAlive() が false を返すので Manager が削除する）
    if (m_Life <= 0.0f)
        return;

    // ---- 放出タイマーを減らす ----
    m_SpawnTimer -= dt;

    // まだ放出タイミングでなければ待機
    if (m_SpawnTimer > 0.0f)
        return;

    // ---- 乱数分布を準備 ----
    std::uniform_real_distribution<float> speedDist(m_Setting.MinSpeed, m_Setting.MaxSpeed);
    std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> lifeDist(m_Setting.MinLife, m_Setting.MaxLife);

    const float interval = 1.0f / static_cast<float>(m_Setting.SpawnPerSec);

    // ---- while ループで1フレーム分まとめて放出 ----
    // SpawnPerSec が高いほど1フレームに多く放出される。
    // 例: SpawnPerSec=5000, dt=0.016 → 1フレームで約80個放出（バースト相当）
    while (m_SpawnTimer <= 0.0f)
    {
        m_SpawnTimer += interval; // 次の放出タイミングを設定

        // グローバルプールから空きスロットを取得（リングバッファ: 常に O(1)）
        ParticleData& p = pool[nextFree];
        nextFree = (nextFree + 1) % poolSize;

        // ランダムな方向と速さを合わせて速度ベクトルを作る（球状放出）
        float speed = speedDist(m_Rng);
        Vector3 dir = { axisDist(m_Rng), axisDist(m_Rng), axisDist(m_Rng) };
        dir.normalize(); // 正規化しないと斜め方向が速くなる

        float life = lifeDist(m_Rng);

        p.Active      = true;
        p.Position    = m_Position;
        p.Velocity    = dir * speed;
        p.LifeTime    = life;
        p.MaxLifeTime = life;
        p.Size        = m_Setting.StartSize;
        p.Rotation    = 0.0f;
        p.Color       = m_Setting.StartColor;
    }
}
