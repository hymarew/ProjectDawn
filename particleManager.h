#pragma once
#include "particleSetting.h"
#include "particleEmitter.h"
#include "particleRenderer.h"
#include <vector>
#include <memory>

// 全パーティクルと全エミッタを一元管理するシングルトンクラス
// 呼び出し側は Emit(EffectType, position) の1行でエフェクトを発生させられる
// パーティクルのプールはこのクラスが単一で保持する（エミッタは持たない）
class ParticleManager
{
public:
    static ParticleManager& GetInstance()
    {
        static ParticleManager s_Instance;
        return s_Instance;
    }

    void Init();
    void Uninit();
    void Update(float dt);
    void Draw();

    // エフェクトを発生させる。ゲームシーンやオブジェクトから呼び出す
    void Emit(EffectType type, Vector3 position);

private:
    ParticleManager()  = default;
    ~ParticleManager() = default;

    // コピー・ムーブ禁止（シングルトン）
    ParticleManager(const ParticleManager&)            = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    static constexpr int POOL_SIZE = 10000; // 同時に存在できるパーティクルの上限

    ParticleData m_Pool[POOL_SIZE]; // グローバルプール（エミッタ間で共有）
    int          m_NextFree = 0;    // リングバッファの次の書き込み位置

    std::vector<std::unique_ptr<ParticleEmitter>> m_Emitters; // アクティブなエミッタ一覧

    ParticleRenderer m_Renderer;

    Vector3 m_Gravity; // 毎フレーム加算する重力加速度ベクトル
};
