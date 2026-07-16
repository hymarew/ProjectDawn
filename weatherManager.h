#pragma once
#include "rainEmitter.h"
#include "snowEmitter.h"
#include "weatherRenderer.h"

// ===================================================
// WeatherManager : 天候システム全体の統括（シングルトン）
//
// 【責務】
//   雨・雪エミッタの更新タイミング管理、カメラ位置の取得と受け渡し、
//   描画データの収集とWeatherRendererへの委譲、ImGuiデバッグパネル。
//   粒の挙動はエミッタ、GPU描画はレンダラーに任せ、自身は調停役に徹する。
//
// 【使い方】（ParticleManagerと同じ流儀）
//   GameScene::Init   : WeatherManager::GetInstance().Init();
//   GameScene::Uninit : WeatherManager::GetInstance().Uninit();
//   GameScene::Update : WeatherManager::GetInstance().Update(dt);
//   GameScene::Draw   : WeatherManager::GetInstance().Draw();  // パーティクル描画の直後
//   ImGui             : Manager::ImGuiDraw から DrawImGui()
// ===================================================
class WeatherManager
{
public:
    static WeatherManager& GetInstance()
    {
        static WeatherManager s_Instance;
        return s_Instance;
    }

    void Init();
    void Uninit();

    // 毎フレーム: カメラ位置を取得し、各エミッタの Spawn→Move→Life更新 を回す
    void Update(float dt);

    // 毎フレーム: アクティブな粒を収集してGPUインスタンシング描画する
    void Draw();

    // "Weather" デバッグパネル（Manager::ImGuiDraw から呼ぶ）
    void DrawImGui();

    RainParams& GetRainParams() { return m_Rain.GetParams(); }
    SnowParams& GetSnowParams() { return m_Snow.GetParams(); }

private:
    WeatherManager()  = default;
    ~WeatherManager() = default;
    WeatherManager(const WeatherManager&)            = delete;
    WeatherManager& operator=(const WeatherManager&) = delete;

    // カメラのワールド位置と右方向ベクトルを取得する（カメラ未生成時はfalse）
    bool GetCameraInfo(Vector3& outPos, Vector3& outRight) const;

    // Field オブジェクトの高さを地面座標として取得する（ParticleManagerと同じ方式）
    float GetGroundY();

    // プールの固定サイズ（要求仕様: 雨5000〜10000本 / 雪3000〜5000枚）
    static constexpr int RAIN_MAX = 10000;
    static constexpr int SNOW_MAX = 5000;

    RainEmitter     m_Rain;
    SnowEmitter     m_Snow;
    WeatherRenderer m_Renderer;

    // 描画用インスタンス配列（capacityを保持して毎フレームの再確保を防ぐ）
    std::vector<WeatherInstance> m_Instances;

    // 地面座標のキャッシュ
    float m_GroundY       = 0.0f;
    bool  m_GroundYCached = false;
};
