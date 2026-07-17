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
#include "particleSystemGPU.h"

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
// CalcSpawnCount : このフレームの放出数を決める共通ロジック
// （寿命・タイマーの消費もここで行う）
// ---------------------------------------------------------
int ParticleEmitter::CalcSpawnCount(float dt)
{
    // ---- バースト放出 ----
    // BurstCount 指定がある場合はフレームレートに依存せず正確な個数を一度に放出し、
    // 役目を終えたエミッタとして即座に寿命を尽きさせる（Manager が削除する）。
    if (m_Setting.BurstCount > 0)
    {
        m_Life = 0.0f;
        return m_Setting.BurstCount;
    }

    // ---- エミッタの寿命を減らす ----
    m_Life -= dt;

    // 寿命が尽きたら何もしない（IsAlive() が false を返すので Manager が削除する）
    if (m_Life <= 0.0f)
        return 0;

    // ---- 放出タイマーを減らす ----
    m_SpawnTimer -= dt;

    // まだ放出タイミングでなければ待機
    if (m_SpawnTimer > 0.0f)
        return 0;

    const float interval = 1.0f / static_cast<float>(m_Setting.SpawnPerSec);

    // ---- while ループで1フレーム分まとめてカウント ----
    // SpawnPerSec が高いほど1フレームに多く放出される。
    // 例: SpawnPerSec=5000, dt=0.016 → 1フレームで約80個放出（バースト相当）
    int count = 0;
    while (m_SpawnTimer <= 0.0f)
    {
        m_SpawnTimer += interval; // 次の放出タイミングを設定
        count++;
    }
    return count;
}

// ---------------------------------------------------------
// Update : 放出タイミングの管理とパーティクルの初期化（CPUシミュレーション）
// ---------------------------------------------------------
int ParticleEmitter::Update(float dt, ParticleData* pool, int poolSize, int& nextFree)
{
    const int count = CalcSpawnCount(dt);
    for (int i = 0; i < count; i++)
        EmitOne(pool, poolSize, nextFree);
    return count;
}

// ---------------------------------------------------------
// UpdateGPU : 放出数だけ計算して EmitRequest を積む（GPUシミュレーション）
// ---------------------------------------------------------
int ParticleEmitter::UpdateGPU(float dt, ParticleSystemGPU& gpu)
{
    const int count = CalcSpawnCount(dt);
    if (count > 0)
        gpu.AddEmitRequest(m_Setting, m_Position, count);
    return count;
}

// ---------------------------------------------------------
// EmitOne : パーティクルを1つ生成してプールへ書き込む
// ---------------------------------------------------------
void ParticleEmitter::EmitOne(ParticleData* pool, int poolSize, int& nextFree)
{
    // ---- 乱数分布を準備 ----
    std::uniform_real_distribution<float> speedDist(m_Setting.MinSpeed, m_Setting.MaxSpeed);
    std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> lifeDist(m_Setting.MinLife, m_Setting.MaxLife);

    // グローバルプールから空きスロットを取得（リングバッファ: 常に O(1)）
    ParticleData& p = pool[nextFree];
    nextFree = (nextFree + 1) % poolSize;

    // ランダムな方向と速さを合わせて速度ベクトルを作る（球状放出）
    float speed = speedDist(m_Rng);
    Vector3 dir = { axisDist(m_Rng), axisDist(m_Rng), axisDist(m_Rng) };
    // 地面衝突するパーティクル（デブリ）は上半球方向に限定し、放物線を描いて飛び散らせる
    if (m_Setting.GroundCollision)
        dir.y = fabsf(dir.y);
    dir.normalize(); // 正規化しないと斜め方向が速くなる

    float life = lifeDist(m_Rng);

    // 生成位置を少しばらつかせる（1点から噴き出す不自然さを消し、千切れた塊の見た目にする）
    Vector3 jitter = { axisDist(m_Rng), axisDist(m_Rng), axisDist(m_Rng) };
    jitter *= m_Setting.PositionJitter;

    // このパーティクル固有の渦回転軸（ランダムな単位ベクトル）
    Vector3 turbAxis = { axisDist(m_Rng), axisDist(m_Rng), axisDist(m_Rng) };
    turbAxis.normalize();

    float sizeMul = 1.0f + axisDist(m_Rng) * m_Setting.SizeVariance; // 個体ごとのサイズばらつき
    float spin    = axisDist(m_Rng) * m_Setting.SpinSpeed;           // ±SpinSpeed の範囲でランダム自転

    p.Active      = true;
    p.Position    = m_Position + jitter;
    p.Velocity    = dir * speed;
    p.LifeTime    = life;
    p.MaxLifeTime = life;
    p.StartSize   = m_Setting.StartSize * sizeMul;
    p.EndSize     = m_Setting.EndSize   * sizeMul;
    p.Size        = p.StartSize;
    p.Rotation    = 0.0f;
    p.SpinRate    = spin;
    p.StartColor  = m_Setting.StartColor;
    p.EndColor    = m_Setting.EndColor;
    p.Color       = m_Setting.StartColor;
    p.Drag            = m_Setting.Drag;
    p.Turbulence      = m_Setting.Turbulence;
    p.TurbulenceAxis  = turbAxis;
    p.BuoyancyDelay   = m_Setting.BuoyancyDelay;
    p.BuoyancyForce   = m_Setting.BuoyancyForce;
    p.TexturePath     = m_Setting.TexturePath;
    p.Additive        = m_Setting.Additive;
    p.GroundAligned   = m_Setting.GroundAligned;
    p.GroundCollision = m_Setting.GroundCollision;
    p.Bounciness      = m_Setting.Bounciness;
}
