#include "particleGPUCommon.hlsl"

// ===================================================
// particleUpdateCS.hlsl
// パーティクル物理演算コンピュートシェーダー（1スレッド = 1粒子）
//
// 【役割】
//   CPU 版 ParticleManager::Update の物理演算をそのまま移植:
//     渦（乱流）→ 減速（Drag）→ 重力/浮力 → 移動 →
//     地面バウンド → 自転 → 寿命 → サイズ/色の補間
//
//   生き残った粒子は描画データを組み立てて、ブレンドモード別の
//   AppendBuffer（通常合成/加算合成）へ詰める。
//   このAppendカウンタがそのまま DrawInstancedIndirect の
//   インスタンス数になるため、CPUへの読み戻しは発生しない。
// ===================================================

cbuffer UpdateParams : register(b0)
{
    float DeltaTime;   // フレーム経過秒
    float GroundY;     // 地面の高さ（デブリのバウンド用）
    float GravityY;    // 重力加速度（下向きなので負値）
    uint  ScanCount;   // 走査するスロット数（CPUが管理するハイウォーターマーク）
};

RWStructuredBuffer<GPUParticle>         Particles    : register(u0); // 粒子プール
AppendStructuredBuffer<ParticleDrawData> DrawListNormal   : register(u1); // 通常合成の描画リスト
AppendStructuredBuffer<ParticleDrawData> DrawListAdditive : register(u2); // 加算合成の描画リスト

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= ScanCount)
        return;

    GPUParticle p = Particles[id.x];

    // 死亡スロットはスキップ（CPU版の !p.Active に相当）
    if (p.LifeTime <= 0.0f)
        return;

    float age = p.MaxLifeTime - p.LifeTime; // 生成からの経過秒数
    float dt  = DeltaTime;

    // ---- 渦（乱流）: 速度に垂直な力で軌道を乱す ----
    if (p.Turbulence > 0.0f)
        p.Velocity += cross(p.TurbulenceAxis, p.Velocity) * (p.Turbulence * dt);

    // ---- 減速（空気抵抗） ----
    if (p.Drag > 0.0f)
        p.Velocity -= p.Velocity * (p.Drag * dt);

    // ---- 重力 or 浮力（BuoyancyDelay 経過で切替） ----
    float3 accel = float3(0.0f, GravityY, 0.0f);
    if (p.BuoyancyDelay >= 0.0f && age > p.BuoyancyDelay)
        accel = float3(0.0f, p.BuoyancyForce, 0.0f);
    p.Velocity += accel * dt;

    // ---- 移動 ----
    p.Position += p.Velocity * dt;

    // ---- 地面バウンド（デブリ用） ----
    if (p.Flags & PARTICLE_FLAG_GROUND_COLLIDE)
    {
        if (p.Position.y <= GroundY && p.Velocity.y < 0.0f)
        {
            p.Position.y  = GroundY;
            p.Velocity.y  = -p.Velocity.y * p.Bounciness; // 反発
            p.Velocity.x *= 0.6f;                          // 摩擦
            p.Velocity.z *= 0.6f;

            if (abs(p.Velocity.y) < 1.0f) // 十分小さければ静止
                p.Velocity = float3(0.0f, 0.0f, 0.0f);
        }
    }

    // ---- 自転 ----
    p.Rotation += p.SpinRate * dt;

    // ---- 寿命 ----
    p.LifeTime -= dt;

    // 書き戻し（死亡した場合も LifeTime<=0 が記録され、次フレームからスキップされる）
    Particles[id.x] = p;

    if (p.LifeTime <= 0.0f)
        return;

    // ---- サイズ・色の補間（CPU版と同じ線形補間） ----
    float t = 1.0f - (p.LifeTime / p.MaxLifeTime);

    ParticleDrawData d;
    d.Position = p.Position;
    d.Size     = lerp(p.StartSize, p.EndSize, t);
    d.Color    = lerp(UnpackColor(p.StartColor), UnpackColor(p.EndColor), t);
    d.Rotation = p.Rotation;
    d.Flags    = p.Flags;
    d.Pad      = float2(0.0f, 0.0f);

    // ブレンドモード別の描画リストへ詰める
    if (p.Flags & PARTICLE_FLAG_ADDITIVE)
        DrawListAdditive.Append(d);
    else
        DrawListNormal.Append(d);
}
