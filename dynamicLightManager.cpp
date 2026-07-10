#include "main.h"
#include "dynamicLightManager.h"
#include "manager.h"
#include "renderer.h"

using namespace DirectX;

DynamicLightManager g_DynamicLightManager;

// GPU側(shader\dynamicLights.hlsl の DynamicLightsBuffer)とレイアウトを一致させること
struct DynamicLightGPU
{
    XMFLOAT4 PositionRadius;
    XMFLOAT4 ColorIntensity;
};
struct DynamicLightsCB
{
    DynamicLightGPU Lights[MAX_DYNAMIC_LIGHTS];
    int             Count;
    float           Pad[3];
};

void DynamicLightManager::Init()
{
    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DEFAULT;
    bd.ByteWidth      = sizeof(DynamicLightsCB);
    bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_Buffer);
}

void DynamicLightManager::Uninit()
{
    if (m_Buffer) { m_Buffer->Release(); m_Buffer = nullptr; }
    for (auto& s : m_Slots) s = Slot{};
}

int DynamicLightManager::FindFreeSlot() const
{
    for (int i = 0; i < MAX_DYNAMIC_LIGHTS; i++)
        if (!m_Slots[i].active) return i;
    return -1;
}

int DynamicLightManager::Acquire(const Vector3& pos, float radius, const Vector3& color, float intensity)
{
    int slot = FindFreeSlot();
    if (slot < 0) return -1; // 上限に達している場合は増やさない（描画負荷を考慮）

    Slot& s     = m_Slots[slot];
    s.active    = true;
    s.position  = pos;
    s.radius    = radius;
    s.color     = color;
    s.intensity = intensity;
    s.isFlash   = false;
    return slot;
}

void DynamicLightManager::UpdateSlot(int slot, const Vector3& pos)
{
    if (slot < 0 || slot >= MAX_DYNAMIC_LIGHTS) return;
    if (!m_Slots[slot].active) return;
    m_Slots[slot].position = pos;
}

void DynamicLightManager::Release(int slot)
{
    if (slot < 0 || slot >= MAX_DYNAMIC_LIGHTS) return;
    m_Slots[slot] = Slot{};
}

void DynamicLightManager::AddFlash(const Vector3& pos, float radius, const Vector3& color, float intensity, float life)
{
    int slot = FindFreeSlot();
    if (slot < 0) return; // 上限に達している場合は諦める（爆発が重なっても破綻しないように）

    Slot& s     = m_Slots[slot];
    s.active    = true;
    s.position  = pos;
    s.radius    = radius;
    s.color     = color;
    s.intensity = intensity;
    s.isFlash   = true;
    s.life      = life;
    s.maxLife   = life;
}

void DynamicLightManager::Update(float dt)
{
    for (auto& s : m_Slots)
    {
        if (!s.active || !s.isFlash) continue;

        s.life -= dt;
        if (s.life <= 0.0f)
            s = Slot{}; // 寿命切れで自動解放
        // 実際の減衰計算（intensity * t^2）は Apply() でGPUへ送る直前にまとめて行う
    }
}

void DynamicLightManager::Apply()
{
    DynamicLightsCB cb{};
    cb.Count = 0;

    // マスタースイッチOFF時はライト0個としてGPUへ送る（影ちかつき等の切り分け用）
    if (!g_DynamicLightsEnabled)
    {
        Renderer::GetDeviceContext()->UpdateSubresource(m_Buffer, 0, nullptr, &cb, 0, 0);
        Renderer::GetDeviceContext()->PSSetConstantBuffers(10, 1, &m_Buffer);
        return;
    }

    for (auto& s : m_Slots)
    {
        if (!s.active) continue;
        if (cb.Count >= MAX_DYNAMIC_LIGHTS) break;

        // 寿命付きライトは残り時間の割合(二乗)で急速に減衰させる
        float intensity = s.intensity;
        if (s.isFlash && s.maxLife > 0.0f)
        {
            float t = s.life / s.maxLife;
            intensity *= t * t;
        }

        DynamicLightGPU& g = cb.Lights[cb.Count];
        g.PositionRadius = { s.position.x, s.position.y, s.position.z, s.radius };
        g.ColorIntensity = { s.color.x, s.color.y, s.color.z, intensity };
        cb.Count++;
    }

    Renderer::GetDeviceContext()->UpdateSubresource(m_Buffer, 0, nullptr, &cb, 0, 0);
    Renderer::GetDeviceContext()->PSSetConstantBuffers(10, 1, &m_Buffer);
}
