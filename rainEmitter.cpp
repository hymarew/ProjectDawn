#include "rainEmitter.h"
#include <cmath>

void RainEmitter::Init(int maxParticles)
{
    // プールを一括確保する（以後は再確保も new/delete もしない）
    m_Pool.assign(maxParticles, RainParticle{});
    m_NextFree         = 0;
    m_ActiveCount      = 0;
    m_SpawnAccumulator = 0.0f;
}

// ---------------------------------------------------------
// Update : Spawn → Move → 寿命更新
// ---------------------------------------------------------
void RainEmitter::Update(float dt, const Vector3& cameraPos, float groundY)
{
    if (!m_Params.Enabled)
    {
        // OFFにしたら残っている粒を即座に消す（プールはそのまま保持）
        if (m_ActiveCount > 0)
        {
            for (auto& p : m_Pool) p.Active = false;
            m_ActiveCount = 0;
        }
        return;
    }

    Spawn(dt, cameraPos);
    Move(dt, cameraPos);
    UpdateLife(dt, groundY);
}

// ---------------------------------------------------------
// Spawn : SpawnRate に従って空きスロットをアクティブ化する
// ---------------------------------------------------------
void RainEmitter::Spawn(float dt, const Vector3& cameraPos)
{
    m_SpawnAccumulator += m_Params.SpawnRate * dt;
    int count = static_cast<int>(m_SpawnAccumulator);
    if (count <= 0) return;
    m_SpawnAccumulator -= static_cast<float>(count);

    const int poolSize = static_cast<int>(m_Pool.size());

    // 空きスロットをリング走査で探す。プールが満杯なら生成を諦める
    // （走査開始位置を覚えておくことで毎回先頭から探すO(n^2)化を防ぐ）
    for (int spawned = 0, scanned = 0; spawned < count && scanned < poolSize; scanned++)
    {
        RainParticle& p = m_Pool[m_NextFree];
        m_NextFree = (m_NextFree + 1) % poolSize;

        if (p.Active) continue;

        ActivateOne(p, cameraPos);
        spawned++;
        m_ActiveCount++;
    }
}

void RainEmitter::ActivateOne(RainParticle& p, const Vector3& cameraPos)
{
    const Vector3& area = m_Params.SpawnArea;

    // カメラ中心の生成領域内のランダム位置。
    // 高さもランダムにすることで、降り始めに「波」ができず自然に見える
    p.Position.x = cameraPos.x + RandSigned() * area.x * 0.5f;
    p.Position.y = cameraPos.y + Rand01()     * area.y;
    p.Position.z = cameraPos.z + RandSigned() * area.z * 0.5f;

    // 真下への落下 + 風による横流れ
    p.Velocity    = MakeWindVelocity(m_Params.WindDirection, m_Params.WindStrength);
    p.Velocity.y  = -m_Params.Speed;

    p.LifeTime = m_Params.LifeTime;
    p.Active   = true;
}

// ---------------------------------------------------------
// Move : 落下＋カメラ追従ラップ
// ---------------------------------------------------------
void RainEmitter::Move(float dt, const Vector3& cameraPos)
{
    for (auto& p : m_Pool)
    {
        if (!p.Active) continue;

        p.Position += p.Velocity * dt;

        // カメラ移動で領域外に出た粒は反対側へラップして密度を保つ
        WrapAroundCamera(p.Position, cameraPos, m_Params.SpawnArea);
    }
}

// ---------------------------------------------------------
// UpdateLife : 地面到達・寿命切れで非アクティブ化（＝再利用可能にする）
// ---------------------------------------------------------
void RainEmitter::UpdateLife(float dt, float groundY)
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
void RainEmitter::FillInstances(std::vector<WeatherInstance>& out,
                                const Vector3& cameraPos,
                                const Vector3& cameraRight) const
{
    (void)cameraPos;
    if (!m_Params.Enabled || m_ActiveCount <= 0) return;

    // 雨粒の画面上での傾き: 風速度をカメラの右方向へ射影した量と
    // 落下速度の比から求める（全粒共通なのでフレームに1回だけ計算）
    Vector3 wind = MakeWindVelocity(m_Params.WindDirection, m_Params.WindStrength);
    const float sideways = wind.x * cameraRight.x + wind.z * cameraRight.z;
    const float lean     = atan2f(sideways, m_Params.Speed);

    for (const auto& p : m_Pool)
    {
        if (!p.Active) continue;

        WeatherInstance inst{};
        inst.Position = { p.Position.x, p.Position.y, p.Position.z };
        inst.Rotation = lean;
        inst.Scale    = { m_Params.Width, m_Params.Length }; // 縦長の板ポリ
        inst.Color    = m_Params.Color;
        out.push_back(inst);
    }
}
