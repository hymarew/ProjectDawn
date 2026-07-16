#include "snowEmitter.h"
#include <cmath>

void SnowEmitter::Init(int maxParticles)
{
    m_Pool.assign(maxParticles, SnowParticle{});
    m_NextFree         = 0;
    m_ActiveCount      = 0;
    m_SpawnAccumulator = 0.0f;
    m_Time             = 0.0f;
}

// ---------------------------------------------------------
// Update : Spawn → Move → 寿命更新
// ---------------------------------------------------------
void SnowEmitter::Update(float dt, const Vector3& cameraPos, float groundY)
{
    if (!m_Params.Enabled)
    {
        if (m_ActiveCount > 0)
        {
            for (auto& p : m_Pool) p.Active = false;
            m_ActiveCount = 0;
        }
        return;
    }

    m_Time += dt;

    Spawn(dt, cameraPos);
    Move(dt, cameraPos);
    UpdateLife(dt, groundY);
}

// ---------------------------------------------------------
// Spawn : SpawnRate に従って空きスロットをアクティブ化する
// ---------------------------------------------------------
void SnowEmitter::Spawn(float dt, const Vector3& cameraPos)
{
    m_SpawnAccumulator += m_Params.SpawnRate * dt;
    int count = static_cast<int>(m_SpawnAccumulator);
    if (count <= 0) return;
    m_SpawnAccumulator -= static_cast<float>(count);

    const int poolSize = static_cast<int>(m_Pool.size());

    for (int spawned = 0, scanned = 0; spawned < count && scanned < poolSize; scanned++)
    {
        SnowParticle& p = m_Pool[m_NextFree];
        m_NextFree = (m_NextFree + 1) % poolSize;

        if (p.Active) continue;

        ActivateOne(p, cameraPos);
        spawned++;
        m_ActiveCount++;
    }
}

void SnowEmitter::ActivateOne(SnowParticle& p, const Vector3& cameraPos)
{
    const Vector3& area = m_Params.SpawnArea;

    // 揺れの基準位置（Anchor）を生成領域内にランダム配置する
    p.AnchorX    = cameraPos.x + RandSigned() * area.x * 0.5f;
    p.AnchorZ    = cameraPos.z + RandSigned() * area.z * 0.5f;
    p.Position.x = p.AnchorX;
    p.Position.y = cameraPos.y + Rand01() * area.y;
    p.Position.z = p.AnchorZ;

    // 落下のみ（横方向はAnchorの風流し＋sin波の揺れで表現するためVelocityには入れない）
    p.Velocity = { 0.0f, -m_Params.Speed, 0.0f };

    p.Size          = m_Params.SizeMin + Rand01() * (m_Params.SizeMax - m_Params.SizeMin);
    p.SwayPhase     = Rand01() * 6.2831853f;                    // 個体ごとに揺れの位相をずらす
    p.Rotation      = Rand01() * 6.2831853f;
    p.RotationSpeed = RandSigned() * m_Params.RotationSpeed;    // ±ランダムの自転
    p.LifeTime      = m_Params.LifeTime;
    p.Active        = true;
}

// ---------------------------------------------------------
// Move : 落下＋風流し＋左右の揺れ＋自転＋カメラ追従ラップ
// ---------------------------------------------------------
void SnowEmitter::Move(float dt, const Vector3& cameraPos)
{
    const Vector3 wind = MakeWindVelocity(m_Params.WindDirection, m_Params.WindStrength);
    const float   freq = m_Params.SwayFrequency * 6.2831853f; // Hz → ラジアン/秒

    for (auto& p : m_Pool)
    {
        if (!p.Active) continue;

        // 揺れの基準位置が風で流される
        p.AnchorX += wind.x * dt;
        p.AnchorZ += wind.z * dt;

        // 落下 + sin波による左右の揺れ（位相は個体ごとにばらす）
        p.Position.y += p.Velocity.y * dt;
        p.Position.x  = p.AnchorX + sinf(m_Time * freq + p.SwayPhase) * m_Params.SwayAmplitude;
        p.Position.z  = p.AnchorZ + sinf(m_Time * freq * 0.7f + p.SwayPhase * 1.3f)
                                   * m_Params.SwayAmplitude * 0.5f; // 奥行き方向は控えめに揺らす

        // ゆっくり自転する
        p.Rotation += p.RotationSpeed * dt;

        // カメラ移動で領域外に出たらAnchorごと反対側へラップする
        Vector3 anchor = { p.AnchorX, p.Position.y, p.AnchorZ };
        WrapAroundCamera(anchor, cameraPos, m_Params.SpawnArea);
        if (anchor.x != p.AnchorX || anchor.z != p.AnchorZ)
        {
            p.Position.x += anchor.x - p.AnchorX;
            p.Position.z += anchor.z - p.AnchorZ;
            p.AnchorX = anchor.x;
            p.AnchorZ = anchor.z;
        }
    }
}

// ---------------------------------------------------------
// UpdateLife : 地面到達・寿命切れで非アクティブ化（＝再利用可能にする）
// ---------------------------------------------------------
void SnowEmitter::UpdateLife(float dt, float groundY)
{
    for (auto& p : m_Pool)
    {
        if (!p.Active) continue;

        p.LifeTime -= dt;
        if (p.LifeTime <= 0.0f || p.Position.y <= groundY)
        {
            p.Active = false;
            m_ActiveCount--;
        }
    }
}

// ---------------------------------------------------------
// FillInstances : 描画用インスタンスデータの詰め込み
// ---------------------------------------------------------
void SnowEmitter::FillInstances(std::vector<WeatherInstance>& out,
                                const Vector3& cameraPos,
                                const Vector3& cameraRight) const
{
    (void)cameraPos;
    (void)cameraRight;
    if (!m_Params.Enabled || m_ActiveCount <= 0) return;

    for (const auto& p : m_Pool)
    {
        if (!p.Active) continue;

        WeatherInstance inst{};
        inst.Position = { p.Position.x, p.Position.y, p.Position.z };
        inst.Rotation = p.Rotation;
        inst.Scale    = { p.Size, p.Size }; // 正方形の板ポリ（丸テクスチャ）
        inst.Color    = m_Params.Color;

        // 寿命の終わり際はソフトにフェードアウトする（急に消えるのを防ぐ）
        const float fadeTime = 1.0f;
        if (p.LifeTime < fadeTime)
            inst.Color.w *= p.LifeTime / fadeTime;

        out.push_back(inst);
    }
}
