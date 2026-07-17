#pragma once
#include <cstdint>
#include <DirectXMath.h>

// =====================================================
// particleGPU.h : GPUパーティクルのCPU/GPU共有データ定義
//
// ここの構造体は shader\particleGPUCommon.hlsl および
// 各CSの定数バッファとバイト単位で一致させること。
// レイアウトを変えたら両方を同時に直す。
// =====================================================

// ---- GPUParticle.Flags のビット割り当て（HLSL側と同値） ----
constexpr uint32_t PARTICLE_FLAG_ADDITIVE       = 1u << 0;  // 加算合成
constexpr uint32_t PARTICLE_FLAG_GROUND_ALIGNED = 1u << 1;  // 地面水平ビルボード
constexpr uint32_t PARTICLE_FLAG_GROUND_COLLIDE = 1u << 2;  // 地面バウンド
constexpr uint32_t PARTICLE_TEX_INDEX_SHIFT     = 8;        // bits8-15: テクスチャ配列インデックス

// ---- 粒子1個のGPU上の状態（96バイト） ----
// CPUはこの構造体を直接読み書きしない（バッファ確保のサイズ計算と
// デバッグ読み戻しにのみ使う）。実体はVRAM上のStructuredBuffer。
struct GPUParticle
{
    DirectX::XMFLOAT3 Position;
    float             LifeTime;       // 残り寿命（秒）。0以下 = 死亡スロット
    DirectX::XMFLOAT3 Velocity;
    float             MaxLifeTime;
    DirectX::XMFLOAT3 TurbulenceAxis;
    float             Turbulence;
    float             StartSize;
    float             EndSize;
    float             Rotation;
    float             SpinRate;
    uint32_t          StartColor;     // R8G8B8A8 パック
    uint32_t          EndColor;
    float             Drag;
    float             BuoyancyDelay;
    float             BuoyancyForce;
    float             Bounciness;
    uint32_t          Flags;
    uint32_t          Pad;
};
static_assert(sizeof(GPUParticle) == 96, "HLSL側 GPUParticle とサイズを一致させること");

// ---- 描画リスト1件分（Update CSが生成し、描画VSが読む。48バイト） ----
struct ParticleDrawData
{
    DirectX::XMFLOAT3 Position;
    float             Size;
    DirectX::XMFLOAT4 Color;
    float             Rotation;
    uint32_t          Flags;
    DirectX::XMFLOAT2 Pad;
};
static_assert(sizeof(ParticleDrawData) == 48, "HLSL側 ParticleDrawData とサイズを一致させること");

// ---- Spawn CS の定数バッファ（particleSpawnCS.hlsl の EmitParams と一致） ----
struct EmitRequestParams
{
    DirectX::XMFLOAT3 EmitPosition;
    float             PositionJitter;

    float MinLife;
    float MaxLife;
    float MinSpeed;
    float MaxSpeed;

    float StartSize;
    float EndSize;
    float SizeVariance;
    float SpinSpeed;

    uint32_t StartColor;
    uint32_t EndColor;
    uint32_t EmitCount;
    uint32_t Seed;

    float Drag;
    float Turbulence;
    float BuoyancyDelay;
    float BuoyancyForce;

    float    Bounciness;
    uint32_t Flags;
    uint32_t PoolSize;
    uint32_t EmitPad;
};
static_assert(sizeof(EmitRequestParams) == 96, "定数バッファは16バイト境界に揃えること");

// ---- Update CS の定数バッファ（particleUpdateCS.hlsl の UpdateParams と一致） ----
struct UpdateParamsGPU
{
    float    DeltaTime;
    float    GroundY;
    float    GravityY;
    uint32_t ScanCount;
};
static_assert(sizeof(UpdateParamsGPU) == 16, "定数バッファは16バイト境界に揃えること");

// XMFLOAT4(0.0〜1.0) を R8G8B8A8 パックカラーへ変換する（HLSL UnpackColor の逆）
inline uint32_t PackColor(const DirectX::XMFLOAT4& c)
{
    auto To8 = [](float v) -> uint32_t
    {
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        return (uint32_t)(v * 255.0f + 0.5f);
    };
    return To8(c.x) | (To8(c.y) << 8) | (To8(c.z) << 16) | (To8(c.w) << 24);
}
