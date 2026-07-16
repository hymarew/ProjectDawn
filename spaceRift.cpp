#include "main.h"
#include "spaceRift.h"
#include "renderer.h"
#include "DirectXTex.h"

namespace
{
    constexpr float DEFAULT_SCALE      = 3.0f;   // 生成時の見た目の直径の基準（ImGuiから調整できる）
    constexpr float GROUND_OFFSET_Y    = 0.02f;  // 地面ちょうどだとZファイティングするため少し浮かせる
    constexpr float GROUND_SCALE_MUL   = 1.2f;   // 地面投影は本体より少し大きめに広げる
    constexpr const wchar_t* CRACK_MASK_PATH = L"asset\\texture\\Crack_Mask.png";
}

void SpaceRift::Init(const Vector3& position)
{
    m_Transform.Position = position;
    m_Transform.Scale    = { DEFAULT_SCALE, DEFAULT_SCALE, DEFAULT_SCALE };

    m_Material.Init();
    m_Renderer.Init();
    m_Distortion.Init();
    m_ParticleEmitter.Init();
    m_Controller.Init();

    TexMetadata  metadata;
    ScratchImage image;
    LoadFromWICFile(CRACK_MASK_PATH, WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(),
        image.GetImageCount(), metadata, &m_CrackMaskSRV);
}

void SpaceRift::Uninit()
{
    if (m_CrackMaskSRV) { m_CrackMaskSRV->Release(); m_CrackMaskSRV = nullptr; }

    m_Renderer.Uninit();
    m_Distortion.Uninit();
    m_Material.Uninit();
}

void SpaceRift::Update(float dt)
{
    m_Controller.Update(dt, m_Material);
    m_ParticleEmitter.Update(dt, m_Transform.Position, m_Controller.IsEnabled());
}

void SpaceRift::Draw()
{
    if (!m_Controller.IsEnabled()) return;

    // 歪ませる前の背景（現時点のバックバッファ）をキャプチャする
    m_Distortion.Capture();

    // 本体（ガラスのような裂け目。背景歪み＋Emissive＋Rim）
    m_Renderer.Draw(m_Transform, m_Material, m_CrackMaskSRV, m_Distortion.GetBackgroundSRV());

    // 地面投影（本体の真下に水平に寝かせ、少し大きめに投影する。Emissiveのみ）
    Transform groundTransform = m_Transform;
    groundTransform.Position.y = GROUND_OFFSET_Y;
    groundTransform.Rotation.x = XM_PIDIV2;
    groundTransform.Scale      = { m_Transform.Scale.x * GROUND_SCALE_MUL,
                                   m_Transform.Scale.y * GROUND_SCALE_MUL, 1.0f };
    m_Renderer.DrawGround(groundTransform, m_Material, m_CrackMaskSRV);
}
