#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "camera.h"
#include "input.h"
#include "inputManager.h"
#include "player.h"
#include "GameConfig.h"

// 各ストラテジーの実装
#include "defaultCameraStrategy.h"
#include "fpsCameraStrategy.h"
#include "tpsCameraStrategy.h"
#include "eventCameraStrategy.h"

Camera* Camera::m_MainCamera = nullptr;

void Camera::Init()
{
    m_Layer      = 0;
    m_Position   = { 0.0f, 1.0f, -10.0f };
    m_MainCamera = this;
    m_CurrentFov = GameConfig::Camera::FOV_TPS;

    m_Projection = XMMatrixPerspectiveFovLH(
        m_CurrentFov,
        (float)SCREEN_WIDTH / SCREEN_HEIGHT, 1.0f, 1500.0f);

    // 初期モードを設定（Strategyを生成）
    SetMode(CameraMode::DEFAULT);
}

void Camera::Uninit()
{
}

void Camera::SetMode(CameraMode mode)
{
    if (m_Strategy) m_Strategy->OnExit();

    switch (mode)
    {
    case CameraMode::DEFAULT: m_Strategy = std::make_unique<DefaultCameraStrategy>(); break;
    case CameraMode::FPS:     m_Strategy = std::make_unique<FpsCameraStrategy>();     break;
    case CameraMode::TPS:     m_Strategy = std::make_unique<TpsCameraStrategy>();     break;
    case CameraMode::EVENT:   m_Strategy = std::make_unique<EventCameraStrategy>();   break;
    }

    m_Strategy->OnEnter(m_Position, m_Target);
    m_Mode = mode;
}

void Camera::Update(float dt)
{
    Player* player = Manager::GetGameObject<Player>();
    if (!player) return;

    // 共通: マウス入力でYaw/Pitch更新
    m_Rotation.y += InputManager::GetCameraMoveX();
    m_Rotation.x += InputManager::GetCameraMoveY();

    // Pitch 制限（TPS以外にも適用）
    if (m_Rotation.x < GameConfig::Camera::TPS_PITCH_MIN) m_Rotation.x = GameConfig::Camera::TPS_PITCH_MIN;
    if (m_Rotation.x > GameConfig::Camera::TPS_PITCH_MAX) m_Rotation.x = GameConfig::Camera::TPS_PITCH_MAX;

    // Strategyに計算を委譲
    CameraContext ctx;
    ctx.playerPos    = player->GetPosition();
    ctx.yaw          = m_Rotation.y;
    ctx.pitch        = m_Rotation.x;
    ctx.dt           = dt;
    ctx.eventTarget  = m_EventTarget;

    CameraOutput out = m_Strategy->Update(ctx);
    m_Position = out.position;
    m_Target   = out.target;
    if (out.fov > 0.0f) m_CurrentFov = out.fov;

    // 共通: ズーム補間
    UpdateZoom(dt);

    // 共通: シェイク
    ApplyShake();

    // デバッグ: Tキーでシェイクテスト
    if (Input::GetKeyTrigger('T'))
        Shake(0.5f, 1.0f);
}

void Camera::UpdateZoom(float dt)
{
    if (m_ZoomTimer >= m_ZoomDuration) return;
    m_ZoomTimer += dt;
    float t = (m_ZoomDuration > 0.0f) ? (m_ZoomTimer / m_ZoomDuration) : 1.0f;
    if (t > 1.0f) t = 1.0f;
    m_CurrentFov = m_CurrentFov + (m_ZoomTargetFov - m_CurrentFov) * t;

    m_Projection = XMMatrixPerspectiveFovLH(
        m_CurrentFov,
        (float)SCREEN_WIDTH / SCREEN_HEIGHT, 1.0f, 1500.0f);
}

void Camera::ApplyShake()
{
    if (m_ShakeTimer <= 0.0f) return;

    float rndX = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float rndY = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float rndZ = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    Vector3 offset = Vector3(rndX, rndY, rndZ) * m_ShakeIntensity;
    m_Position += offset;
    m_Target   += offset;

    m_ShakeIntensity *= GameConfig::Camera::SHAKE_DAMPING;
    m_ShakeTimer     -= GameConfig::Camera::FRAME_TIME;
}

void Camera::ZoomTo(float targetFov, float duration)
{
    m_ZoomTargetFov = targetFov;
    m_ZoomDuration  = duration;
    m_ZoomTimer     = 0.0f;
}

void Camera::Shake(float duration, float intensity)
{
    m_ShakeTimer     = duration;
    m_ShakeIntensity = intensity;
}

void Camera::Draw()
{
    XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);
    m_View = XMMatrixLookAtLH(
        XMLoadFloat3((XMFLOAT3*)&m_Position),
        XMLoadFloat3((XMFLOAT3*)&m_Target),
        XMLoadFloat3(&up));

    Renderer::SetProjectionMatrix(m_Projection);
    Renderer::SetViewMatrix(m_View);
    Renderer::SetCameraPosition(XMFLOAT4(m_Position.x, m_Position.y, m_Position.z, 1.0f));
}

bool Camera::CheckInView(Vector3 pos, float margin)
{
    XMMATRIX viewProj = m_View * m_Projection;
    XMVECTOR vPos     = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
    XMVECTOR vClip    = XMVector4Transform(vPos, viewProj);

    // w≒0（カメラ位置と同一の点など）で除算すると NaN になり、
    // NaN は全ての比較が false になるため「画面内」と誤判定されてしまう。
    // w が正でない = カメラの後方または同一位置なので視界外として扱う。
    float w = XMVectorGetW(vClip);
    if (w <= 0.0001f) return false;

    float x = XMVectorGetX(vClip) / w;
    float y = XMVectorGetY(vClip) / w;
    float z = XMVectorGetZ(vClip) / w;

    if (z < 0.0f || z > 1.0f)         return false;
    if (x < -margin || x > margin)    return false;
    if (y < -margin || y > margin)    return false;
    return true;
}
