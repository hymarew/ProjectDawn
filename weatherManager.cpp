// ===================================================
// weatherManager.cpp
// 天候システム（雨・雪）の統括クラスの実装
// ===================================================

#include "main.h"
#include "weatherManager.h"
#include "manager.h"
#include "camera.h"
#include "field.h"

void WeatherManager::Init()
{
    // プールを固定サイズで一括確保する（以後 new/delete なしで再利用する）
    m_Rain.Init(RAIN_MAX);
    m_Snow.Init(SNOW_MAX);
    m_Renderer.Init(RAIN_MAX + SNOW_MAX);

    // 描画用インスタンス配列も最大数ぶん先に確保しておく（毎フレームの再確保防止）
    m_Instances.reserve(RAIN_MAX + SNOW_MAX);

    m_GroundYCached = false;
}

void WeatherManager::Uninit()
{
    m_Renderer.Uninit();
    m_GroundYCached = false;
}

// ---------------------------------------------------------
// Update : カメラ位置を各エミッタへ渡し、Spawn→Move→Life更新 を回す
// ---------------------------------------------------------
void WeatherManager::Update(float dt)
{
    Vector3 cameraPos, cameraRight;
    if (!GetCameraInfo(cameraPos, cameraRight)) return; // カメラ未生成（シーン構築中）は何もしない

    const float groundY = GetGroundY();

    m_Rain.Update(dt, cameraPos, groundY);
    m_Snow.Update(dt, cameraPos, groundY);
}

// ---------------------------------------------------------
// Draw : 雨→雪の順に収集し、それぞれ DrawInstanced 1回で描画する
// ---------------------------------------------------------
void WeatherManager::Draw()
{
    if (!m_Rain.IsEnabled() && !m_Snow.IsEnabled()) return;

    Vector3 cameraPos, cameraRight;
    if (!GetCameraInfo(cameraPos, cameraRight)) return;

    m_Renderer.ResetStats();

    // ---- 雨: 距離フェード無効（十分遠い値）・縦方向へUVを伸ばす ----
    m_Instances.clear();
    m_Rain.FillInstances(m_Instances, cameraPos, cameraRight);
    if (!m_Instances.empty())
    {
        // UVStretch は長さ/幅の比に応じて丸テクスチャをスジ状に見せる（上限4で破綻を防ぐ）
        const RainParams& rp = m_Rain.GetParams();
        float stretch = (rp.Width > 0.001f) ? rp.Length / (rp.Width * 10.0f) : 2.0f;
        if (stretch < 1.0f) stretch = 1.0f;
        if (stretch > 4.0f) stretch = 4.0f;

        m_Renderer.DrawBatch(m_Instances, 10000.0f, 10001.0f, stretch);
    }

    // ---- 雪: 距離フェード有効・UV等倍 ----
    m_Instances.clear();
    m_Snow.FillInstances(m_Instances, cameraPos, cameraRight);
    if (!m_Instances.empty())
    {
        const SnowParams& sp = m_Snow.GetParams();
        m_Renderer.DrawBatch(m_Instances, sp.FadeStart, sp.FadeEnd, 1.0f);
    }
}

// ---------------------------------------------------------
// GetCameraInfo : カメラのワールド位置と右方向ベクトルを取得する
// ---------------------------------------------------------
bool WeatherManager::GetCameraInfo(Vector3& outPos, Vector3& outRight) const
{
    Camera* camera = Manager::GetGameObject<Camera>();
    if (!camera) return false;

    outPos = camera->GetPosition();

    // 右方向 = 上(Y) × 前方（雨の画面上での傾き計算に使う）
    Vector3 forward = camera->GetForward();
    Vector3 up      = { 0.0f, 1.0f, 0.0f };
    outRight = Vector3::corss(up, forward);
    outRight.normalize();
    return true;
}

// ---------------------------------------------------------
// GetGroundY : Field の高さを地面座標として取得（見つからなければ 0.0f）
// ---------------------------------------------------------
float WeatherManager::GetGroundY()
{
    if (!m_GroundYCached)
    {
        Field* field = Manager::GetGameObject<Field>();
        if (field)
        {
            m_GroundY       = field->GetPosition().y;
            m_GroundYCached = true;
        }
    }
    return m_GroundY;
}

// ---------------------------------------------------------
// DrawImGui : "Weather" デバッグパネル
// ---------------------------------------------------------
void WeatherManager::DrawImGui()
{
    if (!ImGui::CollapsingHeader("Weather")) return;

    ImGui::Text("Rain Active : %d / %d", m_Rain.GetActiveCount(), RAIN_MAX);
    ImGui::Text("Snow Active : %d / %d", m_Snow.GetActiveCount(), SNOW_MAX);
    ImGui::Text("Draw Calls  : %d", m_Renderer.GetDrawCallCount());

    // ---- Rain ----
    ImGui::SeparatorText("Rain");
    RainParams& rain = m_Rain.GetParams();
    ImGui::Checkbox("Rain ON/OFF", &rain.Enabled);
    ImGui::SliderFloat("Rain SpawnRate",     &rain.SpawnRate,     0.0f, 8000.0f);
    ImGui::SliderFloat("Rain Speed",         &rain.Speed,         5.0f, 50.0f);
    ImGui::SliderFloat("Rain WindStrength",  &rain.WindStrength,  0.0f, 15.0f);
    ImGui::SliderFloat("Rain WindDirection", &rain.WindDirection, 0.0f, 360.0f, "%.0f deg");
    ImGui::SliderFloat("Rain Alpha",         &rain.Color.w,       0.0f, 1.0f);
    ImGui::SliderFloat("Rain Length",        &rain.Length,        0.1f, 2.0f);
    ImGui::ColorEdit3 ("Rain Color",         &rain.Color.x);

    // ---- Snow ----
    ImGui::SeparatorText("Snow");
    SnowParams& snow = m_Snow.GetParams();
    ImGui::Checkbox("Snow ON/OFF", &snow.Enabled);
    ImGui::SliderFloat("Snow SpawnRate",    &snow.SpawnRate,    0.0f, 2000.0f);
    ImGui::SliderFloat("Snow Speed",        &snow.Speed,        0.2f, 8.0f);
    ImGui::SliderFloat("Snow WindStrength", &snow.WindStrength, 0.0f, 8.0f);
    ImGui::SliderFloat("Snow SizeMin",      &snow.SizeMin,      0.01f, 0.5f);
    ImGui::SliderFloat("Snow SizeMax",      &snow.SizeMax,      0.01f, 0.8f);
    ImGui::SliderFloat("Snow Alpha",        &snow.Color.w,      0.0f, 1.0f);
    ImGui::ColorEdit3 ("Snow Color",        &snow.Color.x);
}
