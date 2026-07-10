#pragma once

namespace GameConfig
{
    namespace Physics
    {
        constexpr float GRAVITY        = 98.0f;
        constexpr float JUMP_FORCE     = 50.0f;
        constexpr float PLAYER_SPEED   = 100.0f;
        constexpr float SPRINT_MULT    = 1.5f;
        constexpr float RESIST_DIVISOR = 20.0f;
    }

    // =====================================================
    // Particle : パーティクルシステムの調整値
    // 描画はGPUインスタンシング（ドローコールはテクスチャ×ブレンド種別ごとに1回）
    // のため、プール上限を上げても描画コストはほぼ増えない。
    // 更新・描画は使用済みスロット数（ハイウォーターマーク）までしか
    // 走査しないので、上限を大きくしても平常時の負荷は変わらない。
    // =====================================================
    namespace Particle
    {
        constexpr int POOL_SIZE = 1000000; // 同時に存在できるパーティクルの上限（100万）
                                           // プール本体は約150MBをヒープに確保する
    }

    namespace Player
    {
        constexpr float BULLET_SPEED          = 50.0f;
        constexpr float MODEL_ROTATION_OFFSET = 3.14159265f; // XM_PI
        constexpr float MAX_HP                = 100.0f;
        constexpr float INVINCIBLE_SEC        = 1.0f;        // 被弾後の無敵時間（秒）
    }

    namespace Scorpion
    {
        constexpr float HP               = 120.0f;
        constexpr float MOVE_SPEED       =   8.0f;
        constexpr float SENSE_RADIUS     =  50.0f;  // Idle → Chase の索敵半径
        constexpr float LOSE_RADIUS      =  70.0f;  // Chase → Search の見失い半径
        constexpr float ALERT_RADIUS     =  55.0f;  // 発見通知の伝達半径
        constexpr float CHASE_WEIGHT     =   0.7f;  // 追跡方向の重み
        constexpr float SEPARATION_WEIGHT=   0.3f;  // 分離方向の重み
        constexpr float MELEE_DAMAGE     =  20.0f;  // 噛み付き一撃のダメージ
        constexpr float SURROUND_DIST    =   5.0f;  // 囲み補正が発動する距離
        constexpr float SURROUND_OFFSET  =   2.0f;  // 横方向ずれの最大量
    }

    // =====================================================
    // DamageEffect : 被弾時のUI演出調整値（画面全体の赤フラッシュ/カメラシェイクは使わない方針）
    // =====================================================
    namespace DamageEffect
    {
        // HUD全体シェイク
        constexpr float SHAKE_DURATION   = 0.15f; // 揺れる時間（秒）
        constexpr float SHAKE_MAX_OFFSET = 4.0f;  // 最大オフセット（px, X/Y共通）

        // HPゲージ赤フラッシュ
        constexpr float HP_FLASH_DURATION = 0.2f; // 赤→元の色に戻るまでの時間（秒）

        // 被弾時の赤ビネット（0%→35%→0%）
        constexpr float HIT_VIGNETTE_DURATION   = 0.25f;
        constexpr float HIT_VIGNETTE_MAX_ALPHA  = 0.35f;
        constexpr float HIT_VIGNETTE_RISE_RATIO = 0.3f; // durationのうち立ち上がりに使う割合

        // 低HP心拍ビネット（HP30%以下でループ）
        constexpr float LOW_HP_RATIO        = 0.3f;  // これ以下で心拍演出が始まる
        constexpr float HEARTBEAT_PERIOD    = 1.0f;  // 周期（秒）
        constexpr float HEARTBEAT_MIN_ALPHA = 0.15f;
        constexpr float HEARTBEAT_MAX_ALPHA = 0.30f;

        // ビネットの描画幅（画面端からこのピクセル数だけ内側までグラデーションする）
        constexpr float VIGNETTE_SIZE = 220.0f;
    }

    // =====================================================
    // HealItem : 回復アイテムの調整値
    // =====================================================
    namespace HealItem
    {
        constexpr float HEAL_RATIO      = 0.1f;  // 回復量 = 最大HP × この割合
        constexpr float PICKUP_RADIUS   = 1.5f;  // 取得判定の半径（コライダー半径）
        constexpr int   PARTICLE_COUNT  = 30;    // 回復エフェクトの粒子数（20〜40目安）
        constexpr float MODEL_SCALE     = 3.0f;  // HealBox モデルの表示スケール
        constexpr float DROP_RATE       = 0.1f;  // 敵撃破時にドロップする確率（0〜1）
    }

    // =====================================================
    // ScorpionFX : スコーピオンのヒット/撃破エフェクト調整値
    // 「硬い外骨格に弾丸が当たり、削れ、弾かれる」印象を重視。
    // 爆発ではなく火花・装甲片・粉・衝撃波の組み合わせで表現する。
    // =====================================================
    namespace ScorpionFX
    {
        // ---- ヒットフラッシュ（モデル全体の白発光） ----
        constexpr float FLASH_DECAY       = 20.0f; // 通常ヒット: 1.0→0 まで約0.05秒
        constexpr float DEATH_FLASH_DECAY =  6.0f; // 撃破時: 通常より長め（約0.17秒）

        // ---- 撃破演出 ----
        constexpr float DEATH_DURATION = 0.25f; // 撃破からモデル消滅までの時間（秒）

        // ---- 生成数（通常ヒット） ----
        constexpr int HIT_SPARK_COUNT  = 15; // 火花（メイン。12〜20目安）
        constexpr int HIT_DEBRIS_COUNT =  5; // 装甲片（3〜8目安）
        constexpr int HIT_DUST_COUNT   =  3; // 粉

        // ---- 生成数（撃破時） ----
        constexpr int DEATH_SPARK_COUNT  = 30; // 通常の約2倍
        constexpr int DEATH_DEBRIS_COUNT = 18; // 15〜20目安
        constexpr int DEATH_DUST_COUNT   =  8;

        // ---- サイズ倍率（撃破時は通常より大きめ） ----
        constexpr float DEATH_DEBRIS_SIZE_MUL = 1.8f;
        constexpr float DEATH_RING_SIZE_MUL   = 1.6f;
    }

    namespace Collision
    {
        constexpr float SCORPION_RADIUS = 2.5f;
        constexpr float SPIDER_RADIUS   = 2.0f;
        constexpr float PLAYER_RADIUS   = 0.8f;
        constexpr float TREE_RADIUS     = 0.5f; // 木の幹コライダー（SphereCollider）半径
        constexpr float BOX_HALF_SIZE   = 1.5f; // 箱コライダー（BoxCollider）の半サイズ（AABB 各軸）
    }

    // =====================================================
    // Spider : 張り付き型の跳躍アリ。Chase中はScorpionより速く、
    // 一定距離まで詰めるとジャンプで一気に距離を詰める。
    // =====================================================
    namespace Spider
    {
        constexpr float HP                = 80.0f;
        constexpr float MOVE_SPEED        = 12.0f;
        constexpr float SENSE_RADIUS      = 20.0f;  // Idle → Chase の索敵半径
        constexpr float LOSE_RADIUS       = 30.0f;  // Chase → Search の見失い半径
        constexpr float ALERT_RADIUS      = 22.0f;  // 発見通知の伝達半径
        constexpr float CHASE_WEIGHT      =  0.7f;  // 追跡方向の重み
        constexpr float SEPARATION_WEIGHT =  0.3f;  // 分離方向の重み
        constexpr float SEPARATION_RADIUS =  4.0f;  // 近傍蜘蛛との分離半径
        constexpr int   MAX_SEP_CHECKS    =  5;     // 分離計算で確認する近傍数の上限

        constexpr float ATTACK_RANGE      =  8.0f;  // 糸攻撃の射程
        constexpr float STRAFE_DIST       =  4.0f;  // 張り付きストレイフに切り替わる距離
        constexpr float STRAFE_FLIP_TIME  =  0.8f;  // ストレイフの方向転換間隔（秒）
        constexpr float ATTACK_COOLDOWN   =  1.5f;  // 糸攻撃のクールタイム（秒）
        constexpr float SLOW_DURATION     =  2.0f;  // 糸によるスロー効果の持続時間（秒）
        constexpr float SLOW_RATE         =  0.5f;  // スロー中の速度倍率

        constexpr float JUMP_MIN_DIST     =  6.0f;  // ジャンプが発動する最小距離
        constexpr float JUMP_MAX_DIST     = 15.0f;  // ジャンプが発動する最大距離
        constexpr float JUMP_POWER        = 15.0f;  // ジャンプ初速（垂直方向）
        constexpr float JUMP_SPEED        = 10.0f;  // ジャンプ中の水平移動速度
        constexpr float JUMP_COOLDOWN     =  2.0f;  // 連続ジャンプ防止クールタイム（秒）

        constexpr float SEARCH_DURATION   =  4.0f;  // Search ステートの持続時間（秒）
    }

    namespace Bullet
    {
        constexpr float BULLET_RADIUS = 0.1f;
    }

    namespace ScorpionNeedle
    {
        // Behavior 側パラメータ（攻撃タイミング・射程）
        constexpr float FIRE_INTERVAL  =  3.0f;   // 発射間隔（秒）
        constexpr float RANGE_MIN      = 10.0f;   // 有効射程 最小
        constexpr float RANGE_MAX      = 40.0f;   // 有効射程 最大
        constexpr float WINDUP         =  0.5f;   // 発射予兆時間（秒）
    }

    // =====================================================
    // EnemyProjectile : 弾仕様（弾種ごとのパラメータ）
    // Behavior は弾パラメータを直接持たず、ここを参照する。
    // =====================================================
    namespace EnemyProjectile
    {
        constexpr int   POOL_SIZE      = 64;      // プール上限

        // Needle（サソリ毒針）
        constexpr float NEEDLE_SPEED        = 28.8f*1.5f;   // 弾速（m/s）× 2
        constexpr float NEEDLE_DAMAGE       = 10.0f;   // ダメージ
        constexpr float NEEDLE_RADIUS       =  0.5f;   // 当たり判定半径
        constexpr float NEEDLE_LIFE         =  2.0f;   // 弾寿命（秒）
        constexpr int   NEEDLE_COUNT        =  3;      // 同時発射数
        constexpr float NEEDLE_SPREAD_ANGLE =  0.15f;  // 左右の広がり（ラジアン, ~8.6°）
    }

    namespace Game
    {
        constexpr int SCORPION_NUM = 200; // スコーピオンプールの最大数
    }

    namespace Spawner
    {
        constexpr float SPAWN_OFFSET_RANGE   = 5.0f;  // ±5.0f の範囲にランダムスポーン
        constexpr float SPAWN_Y_OFFSET       = 1.0f;
        constexpr float MAX_HP               = 1000.0f;
        constexpr float COLLIDER_RADIUS      = 3.0f;
        constexpr float DEFAULT_SPAWN_RATE   = 2.0f;  // デフォルトのスポーン間隔（体/秒）。2.0f = 0.5秒に1匹、10.0f = 0.1秒に1匹
        constexpr int   BURST_COUNT          = 10;    // バーストスポーン数（この数を生成後にインターバル）
        constexpr float BURST_INTERVAL       = 30.0f; // バースト後の待機時間（秒）
    }

    namespace Camera
    {
        constexpr float SMOOTH_T          = 0.05f;
        constexpr float FPS_HEIGHT        = 1.5f;
        constexpr float TPS_TARGET_HEIGHT = 2.0f;
        constexpr float TPS_DISTANCE      = 5.0f;
        constexpr float TPS_HEIGHT_OFFSET = 1.0f;
        constexpr float DEFAULT_TARGET_Y  = 2.0f;
        constexpr float DEFAULT_DISTANCE  = 10.0f;
        constexpr float DEFAULT_HEIGHT    = 5.0f;
        constexpr float SHAKE_DAMPING     = 0.9f;
        constexpr float FRAME_TIME        = 1.0f / 60.0f;

        // EDF風TPS専用（身長約1.8mのプレイヤーモデル向けに調整）
        constexpr float TPS_BACK_DIST    = 3.5f;    // 後方距離
        constexpr float TPS_HEIGHT_EDF   = 2.2f;    // プレイヤーより上（高くするほど見下ろし角が強くなる）
        constexpr float TPS_LOOKAT_AHEAD = 10.0f;   // 前方注視距離（遠くするほどプレイヤーが画面下へ）
        constexpr float TPS_LOOKAT_UP    = 2.0f;    // 注視点の高さ補正（大きくするほどプレイヤーが画面下へ）
        constexpr float TPS_PITCH_MIN    = -1.047f; // -60°
        constexpr float TPS_PITCH_MAX    =  1.047f; // +60°
        constexpr float TPS_FOLLOW_SPEED = 0.05f;   // 追従補間率
        constexpr float FOV_TPS          = 1.222f;  // 70°
    }

    namespace RocketLauncher
    {
        constexpr int   MAGAZINE_SIZE    = 1;     // 弾倉1発
        constexpr float FIRE_RATE        = 2.0f;  // 2.0秒ごとに1発
        constexpr float RELOAD_TIME      = 2.5f;  // リロード2.5秒
        constexpr float DAMAGE           = 150.0f; // 爆風ダメージ（範囲内全員に適用）
        constexpr float SPEED            = 20.0f;  // ロケット弾速（目で見えるくらい遅い）
        constexpr float LIFE_TIME        = 6.0f;   // 6秒で消滅（射程 = 20*6 = 120）
        constexpr float SPLASH_RADIUS    = 5.0f;   // 爆風半径 5m
        constexpr float KNOCKBACK_POWER  = 500.0f; // 爆発時に敵を外側へ吹き飛ばす力（40 * 2.5）
        constexpr float KNOCKBACK_DECAY  = 8.0f;   // 吹き飛び速度の減衰係数（大きいほど早く止まる）
    }

    // =====================================================
    // RocketFX : ロケットランチャーの発射〜飛行〜爆発演出の調整値
    // =====================================================
    namespace RocketFX
    {
        // ---- 発射時マズルフラッシュ ----
        constexpr float MUZZLE_FLASH_SIZE_MUL = 1.6f; // 通常武器のマズルフラッシュに対する倍率

        // ---- ロケット本体の追従ポイントライト（噴射炎） ----
        constexpr float TRAIL_LIGHT_RADIUS    = 3.0f; // 半径2〜4m
        constexpr float TRAIL_LIGHT_INTENSITY = 0.4f; // 中程度の強さ（周囲の地形でちらついて見えないよう控えめに）

        // ---- 飛行中の火花 ----
        constexpr float SPARK_INTERVAL = 0.15f; // 火花も控えめな頻度で生成する

        // ---- 爆発時の瞬間的なポイントライト ----
        constexpr float EXPLOSION_LIGHT_RADIUS    = 10.0f; // 半径8〜12m
        constexpr float EXPLOSION_LIGHT_INTENSITY = 3.0f;  // 通常より強め
        constexpr float EXPLOSION_LIGHT_LIFE      = 0.12f; // 寿命0.08〜0.15秒
    }

    namespace Explosion
    {
        constexpr int   ANIM_FRAME_MAX  = 16;
        constexpr float ANIM_SHEET_COLS = 4.0f;
        constexpr float ANIM_SHEET_ROWS = 4.0f;
    }

    //namespace Particle
    //{
    //    constexpr int PARTICLE_MAX = 10000;
    //}
}
