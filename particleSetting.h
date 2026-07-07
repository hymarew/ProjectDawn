#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "vector3.h"

using namespace DirectX;

// エフェクト種別
// Emit(EffectType::Explosion, pos) のように呼び出し側が種類を指定する
enum class EffectType
{
    Explosion,      // 爆発
    MuzzleFlash,    // 銃口フラッシュ
    Hit,            // ヒットエフェクト
    Smoke,          // 煙
    Spark,          // 火花
    SpawnerDestroy, // スポナー破壊演出
    BossAppear,     // ボス出現演出
};

// 個々のパーティクルが持つ状態（ParticleManager のグローバルプールで管理）
struct ParticleData
{
    bool     Active;
    Vector3  Position;
    Vector3  Velocity;
    float    LifeTime;     // 残り寿命（秒）
    float    MaxLifeTime;  // 初期寿命（補間の分母として使用）
    float    Size;         // 現在のサイズ
    float    Rotation;     // ビルボード面内の回転（ラジアン）
    XMFLOAT4 Color;        // 現在の色（アルファ含む）
};

// エフェクト1種のパラメータ設定
// コードではなくこの設定データでエフェクトの挙動差を表現する
struct ParticleSetting
{
    float    MinLife;       // 寿命の最小値（秒）
    float    MaxLife;       // 寿命の最大値（秒）
    float    MinSpeed;      // 初速の最小値
    float    MaxSpeed;      // 初速の最大値
    float    StartSize;     // 生成時のサイズ
    float    EndSize;       // 消滅時のサイズ
    XMFLOAT4 StartColor;    // 生成時の色
    XMFLOAT4 EndColor;      // 消滅時の色
    float    EmitterLife;   // エミッタ自体の寿命（秒）
    int      SpawnPerSec;   // 1秒あたりの放出数
    const wchar_t* TexturePath; // 使用テクスチャのパス
};

// 線形補間ヘルパー
inline float LerpF(float a, float b, float t) { return a + (b - a) * t; }
inline XMFLOAT4 LerpColor(const XMFLOAT4& a, const XMFLOAT4& b, float t)
{
    return {
        LerpF(a.x, b.x, t),
        LerpF(a.y, b.y, t),
        LerpF(a.z, b.z, t),
        LerpF(a.w, b.w, t)
    };
}

// ===== エフェクトプリセット =====
// 新しいエフェクトを追加する際はここに関数を追加するだけでよい
namespace ParticlePreset
{
    // 爆発: 短寿命・高速・急速拡大・黄→赤→透明
    inline ParticleSetting Explosion()
    {
        return {
            0.5f, 1.2f,
            5.0f, 15.0f,
            0.5f, 3.0f,
            { 1.0f, 0.8f, 0.2f, 1.0f },
            { 0.8f, 0.1f, 0.0f, 0.0f },
            0.5f, 60,
            L"asset\\texture\\particle.png"
        };
    }

    // 銃口フラッシュ: 超短寿命・白→透明
    inline ParticleSetting MuzzleFlash()
    {
        return {
            0.05f, 0.1f,
            2.0f, 5.0f,
            0.3f, 0.8f,
            { 1.0f, 1.0f, 0.8f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 0.0f },
            0.1f, 30,
            L"asset\\texture\\particle.png"
        };
    }

    // ヒットエフェクト: 短寿命・中速・白→透明
    inline ParticleSetting Hit()
    {
        return {
            0.1f, 0.3f,
            3.0f, 8.0f,
            0.2f, 0.5f,
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 0.0f },
            0.2f, 20,
            L"asset\\texture\\particle.png"
        };
    }

    // 煙: 長寿命・低速・ゆっくり拡大・白→黒透明
    inline ParticleSetting Smoke()
    {
        return {
            3.0f, 5.0f,
            0.5f, 2.0f,
            1.0f, 5.0f,
            { 0.8f, 0.8f, 0.8f, 0.8f },
            { 0.2f, 0.2f, 0.2f, 0.0f },
            3.0f, 5,
            L"asset\\texture\\particle.png"
        };
    }

    // 火花: 超短寿命・高速・全方向放出・オレンジ
    // SpawnPerSec=5000 + EmitterLife=0.05 → 1フレームで約80個のバースト放出
    // StartSize=0.8 → ParticleRenderer が p.Size をそのままスケールに使うため大きめに設定
    inline ParticleSetting Spark()
    {
        return {
            0.2f, 0.5f,      // MinLife, MaxLife（秒）
            8.0f, 22.0f,     // MinSpeed, MaxSpeed
            0.8f, 0.8f,      // StartSize, EndSize（ParticleRenderer のスケールに直結）
            { 1.0f, 0.6f, 0.1f, 1.0f }, // StartColor（オレンジ）
            { 1.0f, 0.2f, 0.0f, 1.0f }, // EndColor（alpha=1固定: discard 対策）
            0.05f, 5000,     // EmitterLife（秒）, SpawnPerSec
            L"asset\\texture\\particle.png"
        };
    }

    // スポナー破壊: 中寿命・中高速・大爆発
    inline ParticleSetting SpawnerDestroy()
    {
        return {
            0.8f, 2.0f,
            3.0f, 12.0f,
            0.8f, 4.0f,
            { 1.0f, 0.6f, 0.0f, 1.0f },
            { 0.5f, 0.0f, 0.0f, 0.0f },
            1.0f, 100,
            L"asset\\texture\\particle.png"
        };
    }

    // ボス出現: 長寿命・低速・大きく広がる・紫
    inline ParticleSetting BossAppear()
    {
        return {
            2.0f, 4.0f,
            1.0f, 5.0f,
            2.0f, 8.0f,
            { 0.5f, 0.0f, 1.0f, 1.0f },
            { 0.1f, 0.0f, 0.3f, 0.0f },
            2.0f, 30,
            L"asset\\texture\\particle.png"
        };
    }
}
