#pragma once
#include <DirectXMath.h>
#include "vector3.h"

using namespace DirectX;

// ===================================================
// weatherSetting.h
// 天候パーティクル（雨・雪）のデータ構造とパラメータ定義
//
// 【データ構造の方針】
//   パーティクルは仮想関数を持たないPOD構造体とし、エミッタごとに
//   固定長の std::vector（Init時に一括確保）へ連続配置する。
//   毎フレームの new/delete は一切行わず、非アクティブなスロットを
//   再利用する ObjectPool 方式で管理する。
// ===================================================

// ---------------------------------------------------
// パーティクル本体（CPU側の状態）
// ---------------------------------------------------

// 全天候パーティクル共通の状態
struct WeatherParticle
{
    Vector3 Position;
    Vector3 Velocity;
    float   LifeTime = 0.0f;  // 残り寿命（秒）。0以下で非アクティブ＝再利用対象
    bool    Active   = false;
};

// 雨粒: 長さ・幅・色は全粒共通（RainParams側）のため追加の状態を持たない
struct RainParticle : WeatherParticle
{
};

// 雪片: 個体ごとにサイズ・揺れの位相・回転を持つ
struct SnowParticle : WeatherParticle
{
    float Size          = 0.1f;
    float SwayPhase     = 0.0f;  // 左右の揺れの位相オフセット（個体差）
    float Rotation      = 0.0f;  // ビルボード面内の回転（ラジアン）
    float RotationSpeed = 0.0f;  // 自転速度（ラジアン/秒。±ランダム）
    float AnchorX       = 0.0f;  // 揺れの中心X（風で流される基準位置）
    float AnchorZ       = 0.0f;  // 揺れの中心Z
};

// ---------------------------------------------------
// エフェクトパラメータ（ImGuiからリアルタイム調整する）
// ---------------------------------------------------

// 雨のパラメータ
struct RainParams
{
    bool    Enabled = false;

    Vector3 SpawnArea = { 30.0f, 20.0f, 30.0f }; // カメラ中心の生成領域（幅×高さ×奥行き, m）
    float   SpawnRate = 3000.0f;                  // 1秒あたりの生成数
    float   Speed     = 22.0f;                    // 落下速度（m/s）
    float   Length    = 0.8f;                     // 雨粒の縦の長さ（m）
    float   Width     = 0.02f;                    // 雨粒の横幅（m）
    float   LifeTime  = 2.0f;                     // 寿命（秒）。地面到達でも消える

    float   WindDirection = 0.0f;                 // 風向き（度。0=+X方向, 反時計回り）
    float   WindStrength  = 2.0f;                 // 風速（m/s）。横に流れる量

    XMFLOAT4 Color = { 0.75f, 0.85f, 1.0f, 0.35f }; // 色（wがAlpha。少しだけ透明な水色）
};

// 雪のパラメータ
struct SnowParams
{
    bool    Enabled = false;

    Vector3 SpawnArea = { 30.0f, 20.0f, 30.0f };
    float   SpawnRate = 600.0f;
    float   Speed     = 1.5f;   // ゆっくり落下（m/s）
    float   SizeMin   = 0.05f;
    float   SizeMax   = 0.16f;
    float   LifeTime  = 10.0f;

    float   WindStrength  = 0.5f;
    float   WindDirection = 0.0f;

    float   SwayAmplitude = 0.5f;  // 左右の揺れ幅（m）
    float   SwayFrequency = 1.0f;  // 揺れの速さ（Hz）
    float   RotationSpeed = 2.0f;  // 自転速度の最大値（ラジアン/秒）

    XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 0.8f }; // 色（wがAlpha）

    float   FadeStart = 15.0f;  // この距離から徐々に透明へ（m）
    float   FadeEnd   = 25.0f;  // この距離で完全に消える（m）
};

// ---------------------------------------------------
// GPU転送用インスタンスデータ（48バイト）
// shader\weatherVS.hlsl の TEXCOORD1〜3 とレイアウトを一致させること
// ---------------------------------------------------
struct WeatherInstance
{
    XMFLOAT3 Position; // ワールド位置
    float    Rotation; // ビルボード面内の回転（雨は風による傾き、雪は自転）
    XMFLOAT2 Scale;    // x=横幅, y=縦の長さ（雨は縦長、雪は正方形）
    XMFLOAT2 Pad;
    XMFLOAT4 Color;    // 色（アルファ含む）
};
