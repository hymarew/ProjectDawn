#pragma once
#include "particleSetting.h"
#include <random>

// 1回分のエフェクト放出を担当するクラス
// 自身は ParticleData を持たず、ParticleManager のグローバルプールに書き込む
class ParticleEmitter
{
public:
    // setting: エフェクトのパラメータ、position: 発生位置
    void Init(const ParticleSetting& setting, Vector3 position);

    // グローバルプールへパーティクルを放出し、自身の寿命を消費する
    // nextFree はリングバッファの書き込み位置。ParticleManager が管理する
    void Update(float dt, ParticleData* pool, int poolSize, int& nextFree);

    bool IsAlive() const { return m_Life > 0.0f; }

private:
    // パーティクルを1つ生成してプールへ書き込む（通常放出・バースト放出の共通処理）
    void EmitOne(ParticleData* pool, int poolSize, int& nextFree);

    ParticleSetting m_Setting;
    Vector3         m_Position;
    float           m_Life;       // エミッタの残り寿命（秒）
    float           m_SpawnTimer; // 次の放出までのカウントダウン（秒）

    std::mt19937    m_Rng; // エミッタごとに独立した乱数エンジン
};
