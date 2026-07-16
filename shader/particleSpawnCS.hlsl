#include "particleGPUCommon.hlsl"

// ===================================================
// particleSpawnCS.hlsl
// パーティクル生成コンピュートシェーダー
//
// 【仕組み】
//   CPU から EmitRequest（プリセット値＋位置＋数＋シード）を
//   定数バッファで受け取り、1スレッド = 1粒子で初期化する。
//
//   空きスロット管理は CPU 版と同じリングバッファ方式:
//   グローバルなカーソルを InterlockedAdd で進め、
//   プールサイズpremod した位置へ上書きする（古い粒子は消える）。
//
//   個体差（方向・速さ・寿命・ジッター等）は
//   「シード + 粒子番号」のハッシュ乱数で生成する。
// ===================================================

// 1回の Emit リクエスト（C++ 側 particleGPU.h の EmitRequestParams と一致させること）
cbuffer EmitParams : register(b0)
{
    float3 EmitPosition;   // 発生位置
    float  PositionJitter; // 生成位置のばらつき半径

    float  MinLife;
    float  MaxLife;
    float  MinSpeed;
    float  MaxSpeed;

    float  StartSize;
    float  EndSize;
    float  SizeVariance;   // ±この割合でサイズをばらつかせる
    float  SpinSpeed;      // ±この値の範囲でランダム自転

    uint   StartColor;     // R8G8B8A8 パック
    uint   EndColor;
    uint   EmitCount;      // このリクエストで生成する粒子数
    uint   Seed;           // 乱数シード（CPUがリクエストごとに変える）

    float  Drag;
    float  Turbulence;
    float  BuoyancyDelay;
    float  BuoyancyForce;

    float  Bounciness;
    uint   Flags;          // PARTICLE_FLAG_* + テクスチャインデックス
    uint   PoolSize;       // リングバッファの全長
    uint   EmitPad;
};

RWStructuredBuffer<GPUParticle> Particles  : register(u0); // 粒子プール
RWByteAddressBuffer             RingCursor : register(u1); // 先頭4バイトが書き込みカーソル

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= EmitCount)
        return;

    // ---- 書き込みスロットの確保（リングバッファ） ----
    // カーソルをatomicに進める。複数スレッド・複数Dispatchが同時に走っても
    // それぞれ重複しないスロットを得られる
    uint slot;
    RingCursor.InterlockedAdd(0, 1, slot);
    slot = slot % PoolSize;

    // ---- 個体差の乱数を用意 ----
    // スレッドごとに異なる初期状態を作る（0にならないようWangHashを通す）
    uint rng = WangHash(Seed + id.x * 1973u + 9277u);

    // ---- 方向と速さ（CPU版 EmitOne と同じ球状放出） ----
    float3 dir = RandVector(rng);
    if (Flags & PARTICLE_FLAG_GROUND_COLLIDE)
        dir.y = abs(dir.y); // デブリは上半球に限定して放物線を描かせる
    dir = normalize(dir + float3(0.0f, 1e-5f, 0.0f)); // 零ベクトル対策の微小値

    float speed = RandRange(rng, MinSpeed, MaxSpeed);
    float life  = RandRange(rng, MinLife, MaxLife);

    // 生成位置のばらつき（1点から噴き出す不自然さを消す）
    float3 jitter = RandVector(rng) * PositionJitter;

    // 渦回転軸（粒子固有のランダム単位ベクトル）
    float3 turbAxis = normalize(RandVector(rng) + float3(0.0f, 1e-5f, 0.0f));

    float sizeMul = 1.0f + RandRange(rng, -1.0f, 1.0f) * SizeVariance;
    float spin    = RandRange(rng, -1.0f, 1.0f) * SpinSpeed;

    // ---- 粒子を初期化してプールへ書き込む ----
    GPUParticle p;
    p.Position       = EmitPosition + jitter;
    p.LifeTime       = life;
    p.Velocity       = dir * speed;
    p.MaxLifeTime    = life;
    p.TurbulenceAxis = turbAxis;
    p.Turbulence     = Turbulence;
    p.StartSize      = StartSize * sizeMul;
    p.EndSize        = EndSize   * sizeMul;
    p.Rotation       = 0.0f;
    p.SpinRate       = spin;
    p.StartColor     = StartColor;
    p.EndColor       = EndColor;
    p.Drag           = Drag;
    p.BuoyancyDelay  = BuoyancyDelay;
    p.BuoyancyForce  = BuoyancyForce;
    p.Bounciness     = Bounciness;
    p.Flags          = Flags;
    p.Pad            = 0u;

    Particles[slot] = p;
}
