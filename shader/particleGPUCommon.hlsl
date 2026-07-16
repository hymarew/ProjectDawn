// ===================================================
// particleGPUCommon.hlsl
// GPUパーティクルの共通定義（Spawn CS / Update CS / 描画VS で共有）
//
// 【レイアウト厳守】
//   GPUParticle / ParticleDrawData は C++ 側 particleGPU.h の
//   構造体とバイト単位で一致させること。
// ===================================================

// ---- GPUParticle.Flags のビット割り当て ----
#define PARTICLE_FLAG_ADDITIVE        (1u << 0)  // 加算合成
#define PARTICLE_FLAG_GROUND_ALIGNED  (1u << 1)  // 地面水平ビルボード
#define PARTICLE_FLAG_GROUND_COLLIDE  (1u << 2)  // 地面バウンド
#define PARTICLE_TEX_INDEX_SHIFT      8          // bits8-15: Texture2DArray のインデックス
#define PARTICLE_TEX_INDEX_MASK       0xFFu

// ---- 粒子1個のGPU上の状態（96バイト。CPUからは書き込まない） ----
struct GPUParticle
{
    float3 Position;        // ワールド位置
    float  LifeTime;        // 残り寿命（秒）。0以下 = 死亡スロット
    float3 Velocity;
    float  MaxLifeTime;     // 初期寿命（補間の分母）
    float3 TurbulenceAxis;  // 渦回転軸（正規化済み）
    float  Turbulence;      // 渦の強さ
    float  StartSize;
    float  EndSize;
    float  Rotation;        // ビルボード面内回転（ラジアン）
    float  SpinRate;        // 自転速度（ラジアン/秒）
    uint   StartColor;      // R8G8B8A8 パック
    uint   EndColor;        // R8G8B8A8 パック
    float  Drag;            // 減速係数
    float  BuoyancyDelay;   // 浮力切替までの秒数（負値で無効）
    float  BuoyancyForce;   // 浮力（上向き加速度）
    float  Bounciness;      // 地面反発係数
    uint   Flags;           // 上記フラグ + テクスチャインデックス
    uint   Pad;
};

// ---- 描画リスト1件分（Update CS が生存粒子から生成する。48バイト） ----
struct ParticleDrawData
{
    float3 Position;
    float  Size;       // 補間済みの現在サイズ
    float4 Color;      // 補間済みの現在色（アルファ含む）
    float  Rotation;
    uint   Flags;      // GroundAligned 判定とテクスチャインデックスに使う
    float2 Pad;
};

// ---------------------------------------------------------
// 乱数（ハッシュベース）
// CPU の mt19937 の代わりに、シード値から決定的に乱数列を作る。
// Spawn CS が「リクエストのシード + 粒子番号」を初期状態にして使う。
// ---------------------------------------------------------
uint WangHash(uint seed)
{
    seed = (seed ^ 61u) ^ (seed >> 16);
    seed *= 9u;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15);
    return seed;
}

// xorshift で状態を進めて [0,1) の乱数を返す
float RandFloat(inout uint state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (state & 0x00FFFFFFu) / 16777216.0f; // 下位24bitを使用
}

float RandRange(inout uint state, float minVal, float maxVal)
{
    return minVal + (maxVal - minVal) * RandFloat(state);
}

// [-1,1] の成分を持つランダムベクトル（CPU版 axisDist と同じ分布）
float3 RandVector(inout uint state)
{
    return float3(RandRange(state, -1.0f, 1.0f),
                  RandRange(state, -1.0f, 1.0f),
                  RandRange(state, -1.0f, 1.0f));
}

// ---------------------------------------------------------
// R8G8B8A8 パックカラーの展開（0.0〜1.0 の float4 へ）
// ---------------------------------------------------------
float4 UnpackColor(uint c)
{
    return float4( (c        & 0xFFu) / 255.0f,
                   ((c >> 8)  & 0xFFu) / 255.0f,
                   ((c >> 16) & 0xFFu) / 255.0f,
                   ((c >> 24) & 0xFFu) / 255.0f );
}
