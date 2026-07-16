#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "vector3.h"
#include "GameConfig.h"

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
    Debris,         // 爆発デブリ（破片）
    ShockwaveRing,  // 爆風リング
};

// 個々のパーティクルが持つ状態（ParticleManager のグローバルプールで管理）
struct ParticleData
{
    bool     Active;
    Vector3  Position;
    Vector3  Velocity;
    float    LifeTime;     // 残り寿命（秒）
    float    MaxLifeTime;  // 初期寿命（補間の分母として使用）
    float    StartSize;    // 生成時のサイズ（個体差を含む）
    float    EndSize;      // 消滅時のサイズ（個体差を含む）
    float    Size;         // 現在のサイズ（StartSize→EndSize を補間した結果）
    float    Rotation;     // ビルボード面内の回転（ラジアン）
    float    SpinRate;     // 自転速度（ラジアン/秒）
    XMFLOAT4 StartColor;   // 生成時の色
    XMFLOAT4 EndColor;     // 消滅時の色
    XMFLOAT4 Color;        // 現在の色（StartColor→EndColor を補間した結果。アルファ含む）
    float    Drag;             // 減速係数（速度に対する割合/秒）。大きいほど早く止まる
    float    Turbulence;       // 渦・乱流の強さ
    Vector3  TurbulenceAxis;   // このパーティクル固有の渦回転軸（正規化済み）
    float    BuoyancyDelay;    // この経過秒数を超えたら重力の代わりに浮力を適用する（-1で無効）
    float    BuoyancyForce;    // 浮力の強さ（上向き加速度）
    const wchar_t* TexturePath = nullptr; // 使用テクスチャのパス（未設定時は描画側のデフォルトを使う）

    bool     Additive        = false; // true: 加算合成（光って見える火花・爆炎・爆風リング向け）
    bool     GroundAligned   = false; // true: カメラ方向ではなく地面に水平な板として描画する（爆風リング用）
    bool     GroundCollision = false; // true: 地面に達したらバウンド／静止する（デブリ用）
    float    Bounciness      = 0.0f;  // 地面衝突時に跳ね返る速度の割合（0〜1）
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

    // ---- 挙動パラメータ（未指定時は旧来どおりの単純な直進運動になる） ----
    float Drag           = 0.0f;  // 減速係数（速度に対する割合/秒）。爆風の勢いが失われる速さ
    float Turbulence     = 0.0f;  // 渦・乱流の強さ。速度に垂直な力を加えて軌道を乱す
    float SpinSpeed      = 0.0f;  // 自転速度の最大値（ラジアン/秒）。±SpinSpeed の範囲でランダム
    float BuoyancyDelay  = -1.0f; // この経過秒数を超えたら重力の代わりに浮力を適用する（-1で無効）
    float BuoyancyForce  = 0.0f;  // 浮力の強さ（上向き加速度）。熱で緩やかに上昇する表現に使う
    float PositionJitter = 0.0f;  // 生成位置のランダムなばらつき（1点から噴き出す不自然さを消す）
    float SizeVariance   = 0.0f;  // 初期・終端サイズのランダムなばらつき（0.3なら±30%）

    // ---- 追加の演出パラメータ（未指定時はすべて無効/0） ----
    bool  Additive        = false; // true: 加算合成で描画する（光るエフェクト用）
    bool  GroundAligned   = false; // true: 地面に水平な板として描画する（ビルボードにしない）
    bool  GroundCollision = false; // true: 地面衝突でバウンド／静止する（デブリ用）
    float Bounciness      = 0.0f;  // 地面衝突時の反発係数（0〜1）

    // 0より大きい場合、SpawnPerSec を無視して初回Updateで正確にこの数だけ放出して終了する。
    // 「火花を必ず15個」のようにフレームレートに依存せず個数を保証したいバースト演出に使う。
    int   BurstCount      = 0;
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
    // 爆発: 爆風で高圧噴出する直後のフェーズ（0〜0.3秒付近）を担当。
    // 高速・強い渦・急激な減速で「放射状に千切れながら広がる」勢いを表現し、
    // 短寿命で燃え尽きるように黄白→赤黒へフェードする。
    inline ParticleSetting Explosion()
    {
        ParticleSetting s{};
        s.MinLife  = 0.5f;  s.MaxLife  = 0.9f;
        s.MinSpeed = 45.0f; s.MaxSpeed = 96.0f; // 高圧で押し出される初速
        s.StartSize = 0.8f; s.EndSize  = 4.5f;  // 急速に体積が増える
        s.StartColor = { 1.0f, 0.9f, 0.5f, 1.0f }; // 黄白（火球）
        s.EndColor   = { 0.5f, 0.1f, 0.02f, 0.0f }; // 赤黒く焦げてフェード
        s.EmitterLife = 0.15f; s.SpawnPerSec = 40000; // 一瞬でまとめて噴き出すバースト
        s.TexturePath = L"asset\\texture\\smoke.png";
        s.Drag           = 3.0f;  // 約0.3秒で勢いが急減する時定数
        s.Turbulence     = 6.0f;  // 強い渦で輪郭が乱れる
        s.SpinSpeed      = 4.0f;  // 速い自転でちぎれる印象を強める
        s.BuoyancyDelay  = -1.0f; // 寿命が短いため浮力フェーズには到達しない
        s.PositionJitter = 0.5f;  // 1点から出ている不自然さを消す
        s.SizeVariance   = 0.4f;  // 塊ごとの大きさをばらつかせる
        s.Additive       = true;  // 火球本体は加算合成で明るく光らせる
        return s;
    }

    // 銃口フラッシュ: 超短寿命・白→透明
    inline ParticleSetting MuzzleFlash()
    {
        return {
            0.05f, 0.1f,
            6.0f, 15.0f,
            0.3f, 0.8f,
            { 1.0f, 1.0f, 0.8f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 0.0f },
            0.1f, 3000,
            L"asset\\texture\\particle.png"
        };
    }

    // ヒットエフェクト: 短寿命・中速・白→透明
    inline ParticleSetting Hit()
    {
        return {
            0.1f, 0.3f,
            9.0f, 24.0f,
            0.2f, 0.5f,
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 0.0f },
            0.2f, 2000,
            L"asset\\texture\\particle.png"
        };
    }

    // 煙: 爆発の続きを引き継ぎ、広がる段階（0.3〜1秒）→立ち上る段階（1秒以降）を担当。
    // 最初は爆風で勢いよく吹き飛ぶが Drag で急減速しながら体積を増し、
    // 1秒経過後は BuoyancyDelay/BuoyancyForce により重力の代わりに緩やかな浮力へ切り替わり、
    // 熱で立ち上りながら拡散・フェードアウトしていく。
    inline ParticleSetting Smoke()
    {
        ParticleSetting s{};
        s.MinLife  = 3.0f; s.MaxLife  = 5.0f;
        s.MinSpeed = 18.0f; s.MaxSpeed = 48.0f; // 爆風で勢いよく吹き飛ぶ初速
        s.StartSize = 1.2f; s.EndSize  = 9.0f; // 膨張してボリュームが増える
        s.StartColor = { 0.85f, 0.82f, 0.78f, 0.9f }; // 密度のある煙
        s.EndColor   = { 0.15f, 0.15f, 0.15f, 0.0f }; // 薄く透明にフェードアウト
        s.EmitterLife = 0.3f; s.SpawnPerSec = 2500;
        s.TexturePath = L"asset\\texture\\smoke.png";
        s.Drag           = 1.2f;  // 速度が急激に落ちる（爆発本体より緩やか）
        s.Turbulence     = 3.5f;  // 渦同士がぶつかり合うような乱流
        s.SpinSpeed      = 2.0f;  // ゆっくり形が崩れていく自転
        s.BuoyancyDelay  = 1.0f;  // 1秒以降は熱で立ち上る
        s.BuoyancyForce  = 1.5f;  // ゆっくりとした上昇（重力9.8に対して控えめ）
        s.PositionJitter = 1.0f;  // 千切れた塊が複数重なる見た目
        s.SizeVariance   = 0.5f;  // 塊ごとの密度ムラ
        return s;
    }

    // 火花: 超短寿命・高速・全方向放出・オレンジ
    // SpawnPerSec=500000 + EmitterLife=0.05 → 1フレームで約8000個のバースト放出
    // StartSize=0.8 → ParticleRenderer が p.Size をそのままスケールに使うため大きめに設定
    inline ParticleSetting Spark()
    {
        ParticleSetting s{};
        s.MinLife  = 0.2f;  s.MaxLife  = 0.5f;
        s.MinSpeed = 24.0f; s.MaxSpeed = 66.0f;
        s.StartSize = 0.8f; s.EndSize  = 0.8f; // ParticleRenderer のスケールに直結
        s.StartColor = { 1.0f, 0.6f, 0.1f, 1.0f }; // オレンジ
        s.EndColor   = { 1.0f, 0.2f, 0.0f, 1.0f }; // alpha=1固定: discard 対策
        s.EmitterLife = 0.05f; s.SpawnPerSec = 500000; // 1フレームで約8000個のバースト放出
        s.TexturePath = L"asset\\texture\\particle.png";
        s.Drag     = 2.0f;  // 空気抵抗で徐々に減速してから重力で落下する
        s.Additive = true;  // 火花は加算合成で明るく光らせる
        return s;
    }

    // スポナー破壊: 中寿命・中高速・大爆発
    inline ParticleSetting SpawnerDestroy()
    {
        return {
            0.8f, 2.0f,
            9.0f, 36.0f,
            0.8f, 4.0f,
            { 1.0f, 0.6f, 0.0f, 1.0f },
            { 0.5f, 0.0f, 0.0f, 0.0f },
            1.0f, 10000,
            L"asset\\texture\\particle.png"
        };
    }

    // ボス出現: 長寿命・低速・大きく広がる・紫
    inline ParticleSetting BossAppear()
    {
        return {
            2.0f, 4.0f,
            3.0f, 15.0f,
            2.0f, 8.0f,
            { 0.5f, 0.0f, 1.0f, 1.0f },
            { 0.1f, 0.0f, 0.3f, 0.0f },
            2.0f, 3000,
            L"asset\\texture\\particle.png"
        };
    }

    // デブリ（破片）: 中速で放物線状に飛散し、重力で落下して地面でバウンド／静止する。
    // GroundCollision により ParticleManager::Update が地面到達を検出してバウンド処理を行う。
    inline ParticleSetting Debris()
    {
        ParticleSetting s{};
        s.MinLife  = 1.5f; s.MaxLife  = 3.0f;
        s.MinSpeed = 18.0f; s.MaxSpeed = 48.0f; // 放物線を描くための初速
        s.StartSize = 0.3f; s.EndSize  = 0.3f; // 大きさは変化させず破片らしく保つ
        s.StartColor = { 0.35f, 0.30f, 0.25f, 1.0f }; // 焦げた土・岩の色
        s.EndColor   = { 0.20f, 0.17f, 0.15f, 0.0f }; // 徐々にフェードアウトして消える
        s.EmitterLife = 0.15f; s.SpawnPerSec = 20000; // 一瞬でまとめて飛び散るバースト
        s.TexturePath = L"asset\\texture\\particle.png";
        s.Drag            = 0.3f;  // 空気抵抗はごく弱い（重い破片のため）
        s.SpinSpeed       = 5.0f;  // 飛びながら不規則に回転する
        s.PositionJitter  = 0.5f;
        s.SizeVariance    = 0.3f;
        s.GroundCollision = true;  // 地面に届いたらバウンド／静止する
        s.Bounciness      = 0.35f; // 反発係数（跳ねるたびに勢いが減っていく）
        return s;
    }

    // =====================================================
    // スコーピオン装甲エフェクト群
    // 「硬い外骨格に弾丸が当たり、削れ、弾かれる」印象を作る。
    // 生成数は引数で渡す（通常ヒットと撃破で使い回すため）。
    // 調整値は GameConfig::ScorpionFX を参照。
    // =====================================================

    // 装甲火花: メインエフェクト。白黄→オレンジで硬い装甲に弾かれた印象を作る。
    inline ParticleSetting ArmorSpark(int count)
    {
        ParticleSetting s{};
        s.MinLife  = 0.15f; s.MaxLife  = 0.4f;
        s.MinSpeed = 6.0f;  s.MaxSpeed = 20.0f; // 弾かれた勢い（ランダム360°）
        s.StartSize = 0.25f; s.EndSize = 0.1f;  // 飛びながら小さくなる
        s.StartColor = { 1.0f, 0.95f, 0.7f, 1.0f }; // 白〜黄
        s.EndColor   = { 1.0f, 0.45f, 0.05f, 0.0f }; // オレンジへ変化しつつフェード
        s.TexturePath = L"asset\\texture\\particle.png";
        s.Drag         = 3.0f;  // 徐々に減速（毎フレーム×0.95相当）
        s.SpinSpeed    = 6.0f;
        s.SizeVariance = 0.5f;
        s.Additive     = true;  // 加算合成で光らせる
        s.BurstCount   = count;
        return s;
    }

    // 装甲片: 削れた外骨格の破片。黒・灰の小片が重力で落ち、回転しながら飛ぶ。
    // white.png（ハードエッジの矩形）を暗く着色することで四角い破片に見せる。
    inline ParticleSetting ArmorDebris(int count, float sizeMul = 1.0f)
    {
        ParticleSetting s{};
        s.MinLife  = 0.6f; s.MaxLife  = 1.2f;
        s.MinSpeed = 4.0f; s.MaxSpeed = 10.0f;
        s.StartSize = 0.14f * sizeMul; s.EndSize = 0.12f * sizeMul;
        s.StartColor = { 0.16f, 0.14f, 0.12f, 1.0f }; // 黒〜焦げ茶
        s.EndColor   = { 0.10f, 0.10f, 0.10f, 0.0f }; // 灰色に沈みながらフェード
        s.TexturePath = L"asset\\texture\\white.png";
        s.Drag            = 0.3f;  // 重い破片なので空気抵抗は弱い（重力で放物線を描く）
        s.SpinSpeed       = 10.0f; // 激しく回転しながら飛ぶ
        s.SizeVariance    = 0.5f;
        s.GroundCollision = true;  // 地面でバウンドして転がる
        s.Bounciness      = 0.3f;
        s.BurstCount      = count;
        return s;
    }

    // 削り粉: 煙ではなく「削れた粉」。命中点から少し広がってすぐ消える。
    inline ParticleSetting ArmorDust(int count)
    {
        ParticleSetting s{};
        s.MinLife  = 0.15f; s.MaxLife  = 0.3f; // 仕様: 0.15〜0.30秒
        s.MinSpeed = 1.0f;  s.MaxSpeed = 3.0f; // 少し広がる程度
        s.StartSize = 0.35f; s.EndSize = 0.9f; // 広がりながら薄れる
        s.StartColor = { 0.5f, 0.45f, 0.38f, 0.55f }; // 灰〜茶の粉
        s.EndColor   = { 0.35f, 0.32f, 0.28f, 0.0f };
        s.TexturePath = L"asset\\texture\\smoke.png";
        s.BuoyancyDelay = 0.0f; s.BuoyancyForce = 0.0f; // 重力無効（その場で漂って消える）
        s.SizeVariance  = 0.4f;
        s.BurstCount    = count;
        return s;
    }

    // 衝撃リング: 命中点で小→大に一瞬で広がる衝撃波（カメラ向きビルボード）。
    inline ParticleSetting ImpactRing(float sizeMul = 1.0f)
    {
        ParticleSetting s{};
        s.MinLife  = 0.15f; s.MaxLife = 0.15f; // 仕様: 約0.15秒
        s.MinSpeed = 0.0f;  s.MaxSpeed = 0.0f; // その場でサイズだけ拡大
        s.StartSize = 0.3f; s.EndSize = 2.2f * sizeMul;
        s.StartColor = { 1.0f, 1.0f, 0.9f, 0.9f };  // 白い閃光
        s.EndColor   = { 1.0f, 0.8f, 0.5f, 0.0f };  // 広がりながら消える
        s.TexturePath = L"asset\\texture\\particle.png";
        s.BuoyancyDelay = 0.0f; s.BuoyancyForce = 0.0f; // 重力無効
        s.Additive   = true;
        s.BurstCount = 1;
        return s;
    }

    // 回復キラキラ: 「生命エネルギーが身体へ流れ込む」柔らかく幻想的な演出。
    // プレイヤー中心から小さな光の粒がふわっと上昇し、少し揺れながら透明になって消える。
    // 緑をメインに黄緑・白を混ぜた色で、加算合成によりほんのり発光する。
    inline ParticleSetting HealSparkle(int count)
    {
        ParticleSetting s{};
        s.MinLife  = 0.8f; s.MaxLife  = 1.2f; // 仕様: 約0.8〜1.2秒
        s.MinSpeed = 0.5f; s.MaxSpeed = 2.0f; // ゆっくり・少しランダムに拡散
        s.StartSize = 0.25f; s.EndSize = 0.08f; // 小さめの光の粒。縮小しながら消える
        s.StartColor = { 0.55f, 1.0f, 0.55f, 1.0f }; // 黄緑〜白っぽい光
        s.EndColor   = { 0.1f,  0.9f, 0.35f, 0.0f }; // 緑に落ち着きながらフェードアウト
        s.TexturePath = L"asset\\texture\\particle.png";
        s.BuoyancyDelay  = 0.0f;  // 重力を無効化し、
        s.BuoyancyForce  = 2.0f;  // 緩やかな上昇力で「ふわっと舞い上がる」動きにする
        s.Turbulence     = 1.5f;  // 少し揺れる
        s.PositionJitter = 0.6f;  // プレイヤーの身体の周囲から湧き出す
        s.SizeVariance   = 0.5f;
        s.Additive       = true;  // 軽い発光（爆発のような強い光にはしない）
        s.BurstCount     = count;
        return s;
    }

    // =====================================================
    // ロケットランチャー演出群（発射〜飛行〜着弾）
    // 生成タイミングは呼び出し側（RocketLauncher / BulletPool）が管理する。
    // 調整値は GameConfig::RocketFX を参照。
    // =====================================================

    // 発射マズルフラッシュ: 通常武器より一回り大きい黄〜オレンジの閃光
    inline ParticleSetting RocketMuzzleFlash()
    {
        ParticleSetting s{};
        const float sizeMul = GameConfig::RocketFX::MUZZLE_FLASH_SIZE_MUL;
        s.MinLife  = 0.05f; s.MaxLife  = 0.1f; // 仕様: 0.05〜0.1秒
        s.MinSpeed = 2.0f;  s.MaxSpeed = 6.0f;
        s.StartSize = 0.35f * sizeMul; s.EndSize = 1.3f * sizeMul; // 徐々に透明になりながら広がる
        s.StartColor = { 1.0f, 0.85f, 0.4f, 1.0f }; // 黄〜オレンジ
        s.EndColor   = { 1.0f, 0.4f,  0.1f, 0.0f };
        s.TexturePath = L"asset\\texture\\particle.png";
        s.Additive   = true;
        s.BurstCount = 6;
        return s;
    }

    // 発射時の煙: マズルフラッシュと同時に発生させる小さな煙（爆発の煙より短寿命・小規模）
    inline ParticleSetting RocketMuzzleSmoke()
    {
        ParticleSetting s = Smoke();
        s.MinLife  = 0.35f; s.MaxLife  = 0.6f;
        s.StartSize = 0.5f; s.EndSize  = 2.0f;
        s.BurstCount = 5;
        return s;
    }

    // 飛行中の火花: 噴射口から控えめに放出する小さな粒子（ロケット本体とは異なる速度で飛ぶ）
    // ※トレイル（発光炎+白煙）は影のちかつきの原因になったため廃止した
    inline ParticleSetting RocketSpark()
    {
        ParticleSetting s{};
        s.MinLife  = 0.2f; s.MaxLife  = 0.5f; // 仕様: 0.2〜0.5秒
        s.MinSpeed = 3.0f; s.MaxSpeed = 8.0f;
        s.StartSize = 0.1f; s.EndSize = 0.03f;
        s.StartColor = { 1.0f, 0.85f, 0.3f,  0.6f }; // 黄〜オレンジ（火花自体は小さいので控えめでも視認できる）
        s.EndColor   = { 1.0f, 0.4f,  0.05f, 0.0f };
        s.TexturePath = L"asset\\texture\\particle.png";
        s.Drag         = 2.0f;
        s.SizeVariance = 0.4f;
        s.Additive     = true;
        s.BurstCount   = 1;
        return s;
    }

    // =====================================================
    // SpaceRift（空間の裂け目）用パーティクル
    // 裂け目の中心から放射状に飛び散る、寿命0.5〜2秒のエネルギー粒子。
    // 青・白の2色をRiftParticleEmitterがランダムに選んで使い分ける。
    // =====================================================

    // 裂け目のエネルギー粒子（青）: 外周のOuterColorと同系統の青
    inline ParticleSetting RiftEnergyBlue()
    {
        ParticleSetting s{};
        s.MinLife  = 0.5f; s.MaxLife  = 2.0f;
        s.MinSpeed = 1.5f; s.MaxSpeed = 6.0f; // 放射状にランダムな速さで飛び散る
        s.StartSize = 0.25f; s.EndSize = 0.02f; // 徐々に小さくなって消える
        s.StartColor = { 0.25f, 0.45f, 1.0f, 1.0f }; // 青
        s.EndColor   = { 0.05f, 0.15f, 0.6f, 0.0f };
        s.TexturePath = L"asset\\texture\\particle.png";
        s.Drag         = 0.8f;  // 空間の抵抗でゆっくり減速する
        s.SizeVariance = 0.4f;
        s.Additive     = true;  // 加算合成でエネルギーが漏れ出る発光感を出す
        return s;
    }

    // 裂け目のエネルギー粒子（白）: 中心のCenterColorに合わせた高輝度の粒
    inline ParticleSetting RiftEnergyWhite()
    {
        ParticleSetting s{};
        s.MinLife  = 0.5f; s.MaxLife  = 2.0f;
        s.MinSpeed = 1.5f; s.MaxSpeed = 6.0f;
        s.StartSize = 0.18f; s.EndSize = 0.02f;
        s.StartColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白
        s.EndColor   = { 0.6f, 0.7f, 1.0f, 0.0f };  // わずかに青みがかってフェード
        s.TexturePath = L"asset\\texture\\particle.png";
        s.Drag         = 0.8f;
        s.SizeVariance = 0.4f;
        s.Additive     = true;
        return s;
    }

    // 爆風リング: 爆心地から地面を這うように高速で拡大し、一瞬で消えるショックウェーブ。
    // GroundAligned によりビルボードではなく地面に水平な板として描画される。
    inline ParticleSetting ShockwaveRing()
    {
        ParticleSetting s{};
        s.MinLife  = 0.3f; s.MaxLife  = 0.3f;
        s.MinSpeed = 0.0f; s.MaxSpeed = 0.0f; // その場に留まり、サイズだけで拡大を表現する
        s.StartSize = 1.0f; s.EndSize  = 20.0f; // 高速で拡大する爆風リング
        s.StartColor = { 1.0f, 0.9f, 0.7f, 0.8f }; // 白っぽい閃光
        s.EndColor   = { 1.0f, 0.5f, 0.2f, 0.0f }; // オレンジに変化しながら消える
        s.EmitterLife = 0.05f; s.SpawnPerSec = 2000; // 約100枚を重ねて生成する
        s.TexturePath = L"asset\\texture\\smoke.png";
        s.BuoyancyDelay  = 0.0f; s.BuoyancyForce = 0.0f; // 重力を実質無効化（地面に張り付いたまま）
        s.Additive       = true; // 加算合成で光るショックウェーブにする
        s.GroundAligned  = true; // カメラ方向ではなく地面に水平に寝かせて描画する
        return s;
    }
}
