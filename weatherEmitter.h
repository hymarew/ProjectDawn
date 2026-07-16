#pragma once
#include "weatherSetting.h"
#include <vector>
#include <random>

// ===================================================
// WeatherEmitter : 天候エミッタの共通インターフェース＋共通処理
//
// 【責務】
//   1種類の天候パーティクル群の「生成(Spawn)→移動(Move)→寿命更新(Life)」を担当する。
//   描画データの詰め込み（FillInstances）までで、GPU描画はWeatherRendererに委譲する。
//
// 【SOLID】
//   WeatherManager はこの抽象クラスにのみ依存する（DIP）。
//   新しい天候（砂嵐・花びら等）を追加するときは派生クラスを1つ作り
//   WeatherManager に登録するだけでよい（OCP）。
//
// 【カメラ追従】
//   パーティクルはカメラ中心の SpawnArea 内にだけ存在する。
//   カメラが移動して領域外へ出た粒は WrapAroundCamera() で反対側へ
//   ラップ（トーラス状に巻き戻し）することで、プレイヤーが高速移動しても
//   天候が途切れず、ワールド全体に生成する必要もない。
// ===================================================
class WeatherEmitter
{
public:
    virtual ~WeatherEmitter() = default;

    // 毎フレーム呼ぶ。内部で Spawn → Move → 寿命更新 の順に処理する
    virtual void Update(float dt, const Vector3& cameraPos, float groundY) = 0;

    // アクティブな粒の描画データを out へ追記する
    // cameraRight: 雨の傾き（画面上での風の流れ）計算に使うカメラの右方向ベクトル
    virtual void FillInstances(std::vector<WeatherInstance>& out,
                               const Vector3& cameraPos,
                               const Vector3& cameraRight) const = 0;

    virtual int  GetActiveCount() const = 0;
    virtual bool IsEnabled()      const = 0;

protected:
    // 風向き（度）と風速から水平の風速度ベクトルを作る
    static Vector3 MakeWindVelocity(float directionDeg, float strength)
    {
        const float rad = directionDeg * (3.14159265f / 180.0f);
        return { cosf(rad) * strength, 0.0f, sinf(rad) * strength };
    }

    // カメラ中心の生成領域からXZ方向に出た粒を反対側へラップする（カメラ追従の核）
    static void WrapAroundCamera(Vector3& pos, const Vector3& cameraPos, const Vector3& area)
    {
        const float halfX = area.x * 0.5f;
        const float halfZ = area.z * 0.5f;

        if      (pos.x >  cameraPos.x + halfX) pos.x -= area.x;
        else if (pos.x <  cameraPos.x - halfX) pos.x += area.x;
        if      (pos.z >  cameraPos.z + halfZ) pos.z -= area.z;
        else if (pos.z <  cameraPos.z - halfZ) pos.z += area.z;
    }

    // 0.0〜1.0 の一様乱数
    float Rand01() { return m_Dist01(m_Rng); }

    // -1.0〜1.0 の一様乱数
    float RandSigned() { return m_Dist01(m_Rng) * 2.0f - 1.0f; }

    std::mt19937 m_Rng{ std::random_device{}() };

    // 生成数の端数持ち越し（SpawnRate×dt が1未満でもフレームレートに依存せず生成できる）
    float m_SpawnAccumulator = 0.0f;

private:
    std::uniform_real_distribution<float> m_Dist01{ 0.0f, 1.0f };
};
