#pragma once
#include "particleSetting.h"
#include <random>

class ParticleSystemGPU;

// 1回分のエフェクト放出を担当するクラス
// 自身は ParticleData を持たず、ParticleManager のグローバルプールに書き込む。
// GPUシミュレーション時は書き込みの代わりに EmitRequest を積む（UpdateGPU）。
class ParticleEmitter
{
public:
    // setting: エフェクトのパラメータ、position: 発生位置
    void Init(const ParticleSetting& setting, Vector3 position);

    // グローバルプールへパーティクルを放出し、自身の寿命を消費する
    // nextFree はリングバッファの書き込み位置。ParticleManager が管理する
    // 戻り値: このフレームで放出した個数（Manager がハイウォーターマークの更新に使う）
    int Update(float dt, ParticleData* pool, int poolSize, int& nextFree);

    // GPUシミュレーション用: 放出タイミングの計算は Update と同じだが、
    // プールへ書き込む代わりに「このフレームの放出数」を EmitRequest として積む。
    // 個々の粒子の初期化（乱数）は Spawn CS が行う
    int UpdateGPU(float dt, ParticleSystemGPU& gpu);

    bool IsAlive() const { return m_Life > 0.0f; }

private:
    // パーティクルを1つ生成してプールへ書き込む（通常放出・バースト放出の共通処理）
    void EmitOne(ParticleData* pool, int poolSize, int& nextFree);

    // このフレームで放出すべき個数を計算し、寿命・タイマーを消費する
    // （Update / UpdateGPU 共通の放出タイミングロジック）
    int CalcSpawnCount(float dt);

    ParticleSetting m_Setting;
    Vector3         m_Position;
    float           m_Life;       // エミッタの残り寿命（秒）
    float           m_SpawnTimer; // 次の放出までのカウントダウン（秒）

    std::mt19937    m_Rng; // エミッタごとに独立した乱数エンジン
};
