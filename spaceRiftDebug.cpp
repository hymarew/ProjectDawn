// ===================================================
// spaceRiftDebug.cpp
// SpaceRiftデバッグデモ（Spawn管理・ImGui操作・Bloom合成込み描画）の実装
// ===================================================

#include "main.h"
#include "spaceRiftDebug.h"

SpaceRiftDebugPanel g_SpaceRiftDebug;

void SpaceRiftDebugPanel::Init()
{
    // BloomPassのGPUリソースはSpawnの有無に関わらず起動時に一度だけ確保する。
    // SpaceRift本体はSpawnボタンを押した時点で初期化する（未使用時のリソースを持たない）
    m_Bloom.Init();
    m_Active = false;
}

void SpaceRiftDebugPanel::Uninit()
{
    if (m_Active) { m_Rift.Uninit(); m_Active = false; }
    m_Bloom.Uninit();
}

void SpaceRiftDebugPanel::Update(float dt)
{
    if (m_Active) m_Rift.Update(dt);
}

void SpaceRiftDebugPanel::Draw()
{
    if (!m_Active) return;

    // 裂け目の発光をMRTのEmissiveバッファへ同時出力し、ぼかして加算合成する
    m_Bloom.BeginEmissivePass();
    m_Rift.Draw();
    m_Bloom.EndEmissivePass();
    m_Bloom.Composite();
}

void SpaceRiftDebugPanel::DrawImGui()
{
    if (!ImGui::CollapsingHeader("Space Rift")) return;

    static float spawnPos[3] = { 0.0f, 3.0f, 10.0f };

    // ---- 未Spawn時: 生成位置の指定とSpawnボタンのみ ----
    if (!m_Active)
    {
        ImGui::InputFloat3("Spawn Position", spawnPos);
        if (ImGui::Button("Spawn"))
        {
            m_Rift.Init({ spawnPos[0], spawnPos[1], spawnPos[2] });
            m_Active = true;
        }
        return;
    }

    // ---- Spawn済み: 破棄・ON/OFF・各種パラメータのリアルタイム調整 ----
    if (ImGui::Button("Destroy"))
    {
        m_Rift.Uninit();
        m_Active = false;
        return;
    }

    ImGui::SameLine();
    bool enabled = m_Rift.IsEnabled();
    if (ImGui::Checkbox("Enabled", &enabled))
        m_Rift.SetEnabled(enabled);

    Transform& tf = m_Rift.GetTransform();
    ImGui::DragFloat3("Position", &tf.Position.x, 0.1f);
    ImGui::DragFloat3("Scale",    &tf.Scale.x,    0.05f, 0.1f, 20.0f);

    RiftMaterial& mat = m_Rift.GetMaterial();
    ImGui::SeparatorText("Emission");
    ImGui::SliderFloat("GlowStrength",       &mat.GlowStrength,       0.0f, 5.0f);
    ImGui::SliderFloat("Bloom Intensity",    &mat.BloomIntensity,     0.0f, 10.0f);
    ImGui::SliderFloat("DistortionStrength", &mat.DistortionStrength, 0.0f, 0.02f);
    ImGui::SliderFloat("RimPower",           &mat.RimPower,           0.5f, 8.0f);
    ImGui::SliderFloat("RimIntensity",       &mat.RimIntensity,       0.0f, 5.0f);
    ImGui::SliderFloat("UVSpeed",            &mat.UVSpeed,            0.0f, 0.2f);
    ImGui::SliderFloat("PulseSpeed",         &mat.PulseSpeed,         0.0f, 6.0f);

    ImGui::SeparatorText("Color");
    ImGui::ColorEdit3("Center (White)", &mat.CenterColor.x);
    ImGui::ColorEdit3("Mid (Purple)",   &mat.MidColor.x);
    ImGui::ColorEdit3("Outer (Blue)",   &mat.OuterColor.x);
    ImGui::ColorEdit3("Rim Cyan",       &mat.RimColorCyan.x);
    ImGui::ColorEdit3("Rim Purple",     &mat.RimColorPurple.x);

    ImGui::SeparatorText("Bloom Pass");
    float bloomRadius    = m_Bloom.GetBlurRadius();
    float bloomIntensity = m_Bloom.GetIntensity();
    if (ImGui::SliderFloat("Blur Radius",         &bloomRadius,    0.5f, 12.0f)) m_Bloom.SetBlurRadius(bloomRadius);
    if (ImGui::SliderFloat("Composite Intensity", &bloomIntensity, 0.0f, 3.0f))  m_Bloom.SetIntensity(bloomIntensity);
}
