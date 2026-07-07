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

#include "particleManager.h"
#include "GameConfig.h"

// ---------------------------------------------------------
// Init : プール・エミッタリストの初期化
// ---------------------------------------------------------
void ParticleManager::Init()
{
    // ---- グローバルプールを全スロット「非アクティブ」で初期化 ----
    // アクティブなスロットだけが Update・Draw で処理される
    for (int i = 0; i < POOL_SIZE; i++)
        m_Pool[i].Active = false;

    // リングバッファの書き込み位置をリセット
    m_NextFree = 0;

    // エミッタのリストをクリア
    m_Emitters.clear();

    // ---- 重力ベクトルを設定 ----
    // GameConfig の重力値を 1/10 スケールで使用
    // （既存の particle.cpp に合わせたスケール）
    m_Gravity = { 0.0f, -GameConfig::Physics::GRAVITY / 10.0f, 0.0f };

    // 描画クラスの初期化（GPUリソースの確保）
    m_Renderer.Init();
}

// ---------------------------------------------------------
// Uninit : リソースの解放
// ---------------------------------------------------------
void ParticleManager::Uninit()
{
    // 描画クラスのGPUリソースを解放
    m_Renderer.Uninit();

    // エミッタのリストをクリア（unique_ptr が自動でメモリ解放）
    m_Emitters.clear();
}

// ---------------------------------------------------------
// Update : エミッタ・パーティクルの更新
// ---------------------------------------------------------
void ParticleManager::Update(float dt)
{
    // ---- 全エミッタを更新 ----
    // 各エミッタが放出タイミングになっていたらグローバルプールにパーティクルを追加する
    for (auto& emitter : m_Emitters)
        emitter->Update(dt, m_Pool, POOL_SIZE, m_NextFree);

    // ---- 寿命が尽きたエミッタをまとめて削除 ----
    // remove_if でリストの末尾に「削除すべきもの」を集め、erase で一括削除する
    m_Emitters.erase(
        std::remove_if(m_Emitters.begin(), m_Emitters.end(),
            [](const std::unique_ptr<ParticleEmitter>& e) { return !e->IsAlive(); }),
        m_Emitters.end()
    );

    // ---- アクティブな全パーティクルの物理演算 ----
    for (int i = 0; i < POOL_SIZE; i++)
    {
        ParticleData& p = m_Pool[i];
        if (!p.Active) continue; // 非アクティブはスキップ

        // 重力を速度に加算（dt をかけることでフレームレートに依存しない）
        p.Velocity += m_Gravity * dt;

        // 速度に従って位置を移動
        p.Position += p.Velocity * dt;

        // 寿命を減らす
        p.LifeTime -= dt;

        // 寿命が尽きたら非アクティブ化
        if (p.LifeTime <= 0.0f)
        {
            p.Active = false;
            continue;
        }

        // ---- アルファ管理 ----
        // unlitTexturePS.hlsl に「alpha < 0.5 → ピクセル破棄（discard）」がある。
        // alpha を下げると途中で突然消えるため、常に 1.0 を維持する。
        // フェードアウトを実装する場合はシェーダーの discard 行を削除すること。
        p.Color.w = 1.0f;

        // TODO: サイズ補間・色補間もここで行いたい
        //       現状は ParticleData が ParticleSetting を参照できないため、
        //       アルファフェードのみ実装している。
        //       将来は ParticleData に settingIndex を持たせて設定を引くと良い。
    }
}

// ---------------------------------------------------------
// Draw : 描画をレンダラーに委譲
// ---------------------------------------------------------
void ParticleManager::Draw()
{
    // ParticleRenderer に全パーティクルの描画を任せる
    m_Renderer.Draw(m_Pool, POOL_SIZE);
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
    default:                         setting = ParticlePreset::Explosion();      break;
    }

    // エミッタを生成してリストに追加
    // unique_ptr を使っているのでメモリ管理は自動
    auto emitter = std::make_unique<ParticleEmitter>();
    emitter->Init(setting, position);
    m_Emitters.push_back(std::move(emitter));
}
