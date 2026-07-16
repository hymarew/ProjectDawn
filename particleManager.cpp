// ===================================================
// particleManager.cpp
// 全パーティクルと全エミッタを一元管理するクラス
//
// 【役割】
//   - パーティクルを格納するグローバルプール（配列）を1つだけ持つ
//   - エミッタ（ParticleEmitter）のリストを管理する
//   - 毎フレーム全パーティクルの物理演算を行う
//   - ParticleRenderer に描画を依頼する
//
// 【使い方】
//   エフェクトを出したいとき：
//     ParticleManager::GetInstance().Emit(EffectType::Spark, 位置);
// ===================================================

#include "main.h"
#include "particleManager.h"
#include "GameConfig.h"
#include "manager.h"
#include "field.h"
#include "explosion.h"
#include <chrono>
#include <algorithm>

namespace
{
    // CPU処理時間の計測ヘルパー（デバッグUIの統計表示用）
    using DebugClock = std::chrono::high_resolution_clock;

    float ElapsedMs(DebugClock::time_point start)
    {
        return std::chrono::duration<float, std::milli>(DebugClock::now() - start).count();
    }
}

// ---------------------------------------------------------
// Init : プール・エミッタリストの初期化
// ---------------------------------------------------------
void ParticleManager::Init()
{
    // ---- グローバルプールの確保（初回のみ） ----
    // 100万個 × 約150バイト ≒ 150MB あるためヒープに置く。
    // シーンをまたいでも確保し直さず使い回す
    if (!m_Pool)
        m_Pool = std::make_unique<ParticleData[]>(POOL_SIZE);

    // ---- グローバルプールを全スロット「非アクティブ」で初期化 ----
    // アクティブなスロットだけが Update・Draw で処理される
    for (int i = 0; i < POOL_SIZE; i++)
        m_Pool[i].Active = false;

    // リングバッファの書き込み位置とハイウォーターマークをリセット
    m_NextFree  = 0;
    m_UsedSlots = 0;

    // エミッタのリストをクリア
    m_Emitters.clear();

    // ---- 重力ベクトルを設定 ----
    // GameConfig の重力値を 1/10 スケールで使用
    // （既存の particle.cpp に合わせたスケール）
    m_Gravity = { 0.0f, -GameConfig::Physics::GRAVITY / 10.0f, 0.0f };

    // 描画クラスの初期化（GPUリソースの確保）
    // インスタンスバッファはプール全量ぶんの容量を確保する
    m_Renderer.Init(POOL_SIZE);

    // GPUシミュレーション一式の初期化。
    // 失敗した場合（リソース確保不能など）は m_UseGPU が常に false になり
    // CPU経路のみで動作する（フォールバック）
    m_GPU.Init(POOL_SIZE);
}

// ---------------------------------------------------------
// Uninit : リソースの解放
// ---------------------------------------------------------
void ParticleManager::Uninit()
{
    // 描画クラスのGPUリソースを解放
    m_Renderer.Uninit();

    // GPUシミュレーションのリソースを解放
    m_GPU.Uninit();

    // エミッタのリストをクリア（unique_ptr が自動でメモリ解放）
    m_Emitters.clear();
}

// ---------------------------------------------------------
// Update : エミッタ・パーティクルの更新
// ---------------------------------------------------------
void ParticleManager::Update(float dt)
{

    const auto updateStart = DebugClock::now(); // 統計用: シミュレーション時間の計測開始

    // ---- 画面フラッシュのタイマーを更新 ----
    if (m_FlashTimer > 0.0f)
    {
        m_FlashTimer -= dt;
        if (m_FlashTimer < 0.0f) m_FlashTimer = 0.0f;
    }

    // ---- GPUシミュレーション経路 ----
    // エミッタは「何個出すか」を EmitRequest として積むだけで、
    // 粒子の初期化・物理演算・描画リスト構築はすべて m_GPU（CS）が行う。
    // ここで計測される UpdateMs は Dispatch の発行コストのみになる
    if (m_UseGPU)
    {
        for (auto& emitter : m_Emitters)
            emitter->UpdateGPU(dt, m_GPU);

        m_Emitters.erase(
            std::remove_if(m_Emitters.begin(), m_Emitters.end(),
                [](const std::unique_ptr<ParticleEmitter>& e) { return !e->IsAlive(); }),
            m_Emitters.end()
        );

        m_GPU.Update(dt, GetGroundY(), m_Gravity.y);

        m_Stats.UpdateMs    = ElapsedMs(updateStart);
        m_Stats.UsedSlots   = m_GPU.GetScanCount();
        m_Stats.ActiveCount = m_GPU.GetActiveCount(); // 1〜2フレーム遅れの参考値
        return;
    }

    // ---- 全エミッタを更新 ----
    // 各エミッタが放出タイミングになっていたらグローバルプールにパーティクルを追加する
    const int prevNextFree   = m_NextFree;
    int       emittedTotal   = 0;
    for (auto& emitter : m_Emitters)
        emittedTotal += emitter->Update(dt, m_Pool.get(), POOL_SIZE, m_NextFree);

    // ---- ハイウォーターマーク（走査範囲）を進める ----
    // 書き込み位置がリングバッファを一周した場合は全域を使用中とみなす。
    // そうでなければ「書き込み位置の末尾」まで走査範囲を広げる
    if (emittedTotal >= POOL_SIZE || (emittedTotal > 0 && m_NextFree <= prevNextFree))
        m_UsedSlots = POOL_SIZE;
    else if (m_NextFree > m_UsedSlots)
        m_UsedSlots = m_NextFree;

    // ---- 寿命が尽きたエミッタをまとめて削除 ----
    // remove_if でリストの末尾に「削除すべきもの」を集め、erase で一括削除する
    m_Emitters.erase(
        std::remove_if(m_Emitters.begin(), m_Emitters.end(),
            [](const std::unique_ptr<ParticleEmitter>& e) { return !e->IsAlive(); }),
        m_Emitters.end()
    );
    // ---- アクティブな全パーティクルの物理演算 ----
    // 書き込んだことのあるスロット（ハイウォーターマーク）までで走査を打ち切る。
    // 併せて「最後にアクティブだったスロット位置」を記録し、ループ後に走査範囲を縮める。
    // これがないと大量バースト後にプール末尾まで走査し続け、FPSが戻らなくなる
    int lastActive = -1;
    int loop = 0;
    for (int i = 0; i < m_UsedSlots; i++)
    {
        loop++;
        ParticleData& p = m_Pool[i];
        if (!p.Active) continue; // 非アクティブはスキップ

        float age = p.MaxLifeTime - p.LifeTime; // 生成からの経過秒数

        // ---- 渦（乱流） ----
        // 速度に垂直な力を加えて軌道を乱し、不規則な渦を巻きながら形が崩れる見た目にする。
        // 速度が Drag で落ちるほどこの影響も小さくなり、自然と乱流が収まっていく。
        if (p.Turbulence > 0.0f)
        {
            Vector3 axis = p.TurbulenceAxis;
            Vector3 vel  = p.Velocity;
            p.Velocity += Vector3::corss(axis, vel) * p.Turbulence * dt;
        }

        // ---- 減速（空気抵抗） ----
        // 爆風の勢いが時間とともに失われていく表現
        if (p.Drag > 0.0f)
            p.Velocity -= p.Velocity * (p.Drag * dt);

        // ---- 重力 or 浮力 ----
        // 一定時間（BuoyancyDelay）が経過すると、重力の代わりに緩やかな上向きの浮力に切り替わる。
        // これにより「爆風で吹き飛ぶ→熱でゆっくり立ち上る」という2段階の動きになる。
        Vector3 accel = m_Gravity;
        if (p.BuoyancyDelay >= 0.0f && age > p.BuoyancyDelay)
            accel = { 0.0f, p.BuoyancyForce, 0.0f };
        p.Velocity += accel * dt;

        // 速度に従って位置を移動
        p.Position += p.Velocity * dt;

        // ---- 地面衝突（デブリ用） ----
        // 地面まで落ちてきたらバウンドし、勢いが十分小さくなったら静止する。
        if (p.GroundCollision)
        {
            float groundY = GetGroundY();
            if (p.Position.y <= groundY && p.Velocity.y < 0.0f)
            {
                p.Position.y  = groundY;
                p.Velocity.y  = -p.Velocity.y * p.Bounciness; // 反発
                p.Velocity.x *= 0.6f; // 摩擦で水平方向の勢いも減衰
                p.Velocity.z *= 0.6f;

                if (fabsf(p.Velocity.y) < 1.0f) // 跳ね返りが十分小さければ静止させる
                    p.Velocity = { 0.0f, 0.0f, 0.0f };
            }
        }

        // 自転（ビルボード面内の回転）。渦を巻いて形が崩れて見える効果を補強する
        p.Rotation += p.SpinRate * dt;

        // 寿命を減らす
        p.LifeTime -= dt;

        // 寿命が尽きたら非アクティブ化
        if (p.LifeTime <= 0.0f)
        {
            p.Active = false;
            continue;
        }

        // ---- サイズ・色（アルファ含む）の補間 ----
        // 寿命の進行度(0=生成直後 → 1=消滅直前)に応じて Start→End を線形補間する。
        float t = 1.0f - (p.LifeTime / p.MaxLifeTime);
        p.Size  = LerpF(p.StartSize, p.EndSize, t);
        p.Color = LerpColor(p.StartColor, p.EndColor, t);

        lastActive = i; // ここまで生き残った = 走査が必要な末尾候補
    }
    // ---- 走査範囲の縮小 ----
    // 末尾側の死んだ領域を切り捨てる。大量バーストの粒子が消えれば
    // 走査範囲も自動で縮み、通常プレイのコストに戻る
    m_UsedSlots = lastActive + 1;

    m_Stats.UpdateMs  = ElapsedMs(updateStart); // 統計用: シミュレーション時間を記録
    m_Stats.UsedSlots = m_UsedSlots;            // 統計用: 現在の走査範囲
    m_NextFree = m_UsedSlots;
}

// ---------------------------------------------------------
// Draw : 描画をレンダラーに委譲
// ---------------------------------------------------------
void ParticleManager::Draw()
{
    const auto drawStart = DebugClock::now(); // 統計用: 描画CPU時間の計測開始

    if (m_UseGPU)
    {
        // GPU経路: 描画リストは Update CS が構築済み。間接描画を発行するだけ
        m_GPU.Draw();
        m_Stats.DrawMs    = ElapsedMs(drawStart);
        m_Stats.DrawCalls = 2; // 通常合成 + 加算合成
        return;
    }

    // ParticleRenderer に全パーティクルの描画を任せる
    // （走査はハイウォーターマークまで。未使用領域は見に行かない）
    m_Renderer.Draw(m_Pool.get(), m_UsedSlots);

    // 統計を回収（デバッグUIが GetStats() で参照する）
    m_Stats.DrawMs      = ElapsedMs(drawStart);
    m_Stats.ActiveCount = m_Renderer.GetActiveCount();
    m_Stats.DrawCalls   = m_Renderer.GetDrawCallCount();
}

// ---------------------------------------------------------
// Emit : エフェクトを発生させる
// ---------------------------------------------------------
void ParticleManager::Emit(EffectType type, Vector3 position)
{
    // ---- エフェクト種別からプリセット設定を取得 ----
    // 新しいエフェクトを追加したいときは particleSetting.h の
    // ParticlePreset 名前空間に関数を追加し、ここに case を1行足すだけでよい
    ParticleSetting setting;
    switch (type)
    {
    case EffectType::Explosion:      setting = ParticlePreset::Explosion();      break;
    case EffectType::MuzzleFlash:    setting = ParticlePreset::MuzzleFlash();    break;
    case EffectType::Hit:            setting = ParticlePreset::Hit();            break;
    case EffectType::Smoke:          setting = ParticlePreset::Smoke();          break;
    case EffectType::Spark:          setting = ParticlePreset::Spark();          break;
    case EffectType::SpawnerDestroy: setting = ParticlePreset::SpawnerDestroy(); break;
    case EffectType::BossAppear:     setting = ParticlePreset::BossAppear();     break;
    case EffectType::Debris:         setting = ParticlePreset::Debris();         break;
    case EffectType::ShockwaveRing:  setting = ParticlePreset::ShockwaveRing();  break;
    default:                         setting = ParticlePreset::Explosion();      break;
    }

    Emit(setting, position);
}

// ---------------------------------------------------------
// Emit : プリセットを直接指定してエフェクトを発生させる
// ---------------------------------------------------------
void ParticleManager::Emit(const ParticleSetting& setting, Vector3 position)
{
    // エミッタを生成してリストに追加
    // unique_ptr を使っているのでメモリ管理は自動
    auto emitter = std::make_unique<ParticleEmitter>();
    emitter->Init(setting, position);
    m_Emitters.push_back(std::move(emitter));
}

// ---------------------------------------------------------
// EmitScorpionHit : スコーピオン被弾演出
// 「硬い装甲に弾丸が当たり、削れ、弾かれる」を複数の小エフェクトの重ねで表現する。
// ヒットフラッシュ（モデルの白発光）は Scorpion 側のシェーダーが担当する。
// ---------------------------------------------------------
void ParticleManager::EmitScorpionHit(Vector3 position)
{
    using namespace GameConfig;
    Emit(ParticlePreset::ArmorSpark (ScorpionFX::HIT_SPARK_COUNT),  position); // 火花（メイン）
    Emit(ParticlePreset::ArmorDebris(ScorpionFX::HIT_DEBRIS_COUNT), position); // 装甲片
    Emit(ParticlePreset::ArmorDust  (ScorpionFX::HIT_DUST_COUNT),   position); // 削り粉
    Emit(ParticlePreset::ImpactRing (1.0f),                          position); // 衝撃リング
}

// ---------------------------------------------------------
// EmitScorpionDeath : スコーピオン撃破演出
// 爆発は使わず、装甲が限界を迎えて砕け散るイメージ。通常ヒットの強化版。
// ---------------------------------------------------------
void ParticleManager::EmitScorpionDeath(Vector3 position)
{
    using namespace GameConfig;
    Emit(ParticlePreset::ArmorSpark (ScorpionFX::DEATH_SPARK_COUNT),  position);
    Emit(ParticlePreset::ArmorDebris(ScorpionFX::DEATH_DEBRIS_COUNT,
                                     ScorpionFX::DEATH_DEBRIS_SIZE_MUL), position);
    Emit(ParticlePreset::ArmorDust  (ScorpionFX::DEATH_DUST_COUNT),   position);
    Emit(ParticlePreset::ImpactRing (ScorpionFX::DEATH_RING_SIZE_MUL), position);
}

// ---------------------------------------------------------
// EmitBigExplosion : 爆発演出一式（発光フラッシュ・火球・火花・デブリ・煙・爆風リング）を発生させる
// ---------------------------------------------------------
void ParticleManager::EmitBigExplosion(Vector3 position)
{
    // 1. 発光フラッシュ：画面全体を一瞬明るくする + 爆心地の強い発光（加算合成の火球パーティクルが担う）
    TriggerScreenFlash(0.12f);

    // 2. 火球：既存のスプライトアニメーション（Explosion.png）を大きめのスケールで再生
    Explosion* fireball = Manager::AddGameObject<Explosion>();
    fireball->SetPosition(position);
    fireball->SetScale({ 8.0f, 8.0f, 8.0f });

    // 3. 爆風リング：爆心地の高さに関わらず、常に地面の高さで這うように拡大させる
    Vector3 ringPosition = position;
    ringPosition.y = GetGroundY();
    Emit(EffectType::ShockwaveRing, ringPosition);

    // 4. 火球パーティクル：加算合成の炎の塊（発光フラッシュの一部も兼ねる）
    Emit(EffectType::Explosion, position);

    // 5. 火花：全方向へ飛び散り、重力で落下・減速しながら消えていく
    Emit(EffectType::Spark, position);

    // 6. デブリ（破片）：放物線を描いて飛び散り、地面でバウンド／静止する
    Emit(EffectType::Debris, position);

    // 7. 煙：爆風で勢いよく広がった後、ゆっくり立ち上りながら消えていく
    Emit(EffectType::Smoke, position);
}

// ---------------------------------------------------------
// EmitHeal : 回復演出
// 「生命エネルギーが身体へ流れ込む」柔らかい演出。爆発や煙は使わない。
// ---------------------------------------------------------
void ParticleManager::EmitHeal(Vector3 position)
{
    Emit(ParticlePreset::HealSparkle(GameConfig::WorldItem::HEAL_PARTICLE_COUNT), position);
}

// ---------------------------------------------------------
// TriggerScreenFlash : 画面全体を一瞬明るくするフラッシュを開始する
// ---------------------------------------------------------
void ParticleManager::TriggerScreenFlash(float duration)
{
    m_FlashDuration = duration;
    m_FlashTimer    = duration;
}

// ---------------------------------------------------------
// DrawScreenFlash : 画面フラッシュの描画（ImGui::Render() の直前に呼ぶ）
// ---------------------------------------------------------
void ParticleManager::DrawScreenFlash()
{
    if (m_FlashTimer <= 0.0f || m_FlashDuration <= 0.0f) return;

    // 経過とともに急速に減衰させる（発生直後が最も明るい）
    float t     = m_FlashTimer / m_FlashDuration;
    float alpha = t * t;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImU32 col = IM_COL32(255, 255, 255, (int)(alpha * 255.0f));
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        col);
}

// ---------------------------------------------------------
// GetGroundY : Field オブジェクトの高さを地面座標として取得する（見つからなければ 0.0f）
// ---------------------------------------------------------
float ParticleManager::GetGroundY()
{
    if (!m_GroundYCached)
    {
        Field* field = Manager::GetGameObject<Field>();
        if (field)
        {
            m_GroundY       = field->GetPosition().y;
            m_GroundYCached = true;
        }
    }
    return m_GroundY;
}
