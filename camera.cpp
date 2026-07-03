#include "main.h"
#include "renderer.h"
#include "camera.h"
#include "input.h"
#include "inputManager.h"

namespace
{
    constexpr float MOVE_SPEED    = 10.0f;   // カメラ自由移動の速度
    constexpr float PITCH_MIN     = -1.4f;
    constexpr float PITCH_MAX     = 1.4f;
    constexpr float SHAKE_DAMPING = 0.9f;
}

void Camera::Init()
{
    m_Layer         = 0;
    m_Position      = { 0.0f, 5.0f, -10.0f };
    m_CurrentFov    = 1.222f;  // 70°
    m_ZoomTargetFov = m_CurrentFov;

    m_Projection = XMMatrixPerspectiveFovLH(
        m_CurrentFov,
        (float)SCREEN_WIDTH / SCREEN_HEIGHT, 1.0f, 1000.0f);

    m_Target = m_Position + GetForward();
}

void Camera::Uninit()
{
}

void Camera::Update(float dt)
{
    // マウス/スティックで視点回転
    m_Rotation.y += InputManager::GetCameraMoveX();
    m_Rotation.x += InputManager::GetCameraMoveY();

    if (m_Rotation.x < PITCH_MIN) m_Rotation.x = PITCH_MIN;
    if (m_Rotation.x > PITCH_MAX) m_Rotation.x = PITCH_MAX;

    // WASD/スティックで自由移動（追従対象のPlayerがまだ無いため）
    Vector3 forward = GetForward();
    Vector3 right   = GetRight();
    float moveX = InputManager::GetMoveX();
    float moveY = InputManager::GetMoveY();

    m_Position += right   * (moveX * MOVE_SPEED * dt);
    m_Position += forward * (moveY * MOVE_SPEED * dt);

    m_Target = m_Position + forward;

    // ズーム補間
    UpdateZoom(dt);

    // シェイク
    ApplyShake(dt);

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
        (float)SCREEN_WIDTH / SCREEN_HEIGHT, 1.0f, 1000.0f);
}

void Camera::ApplyShake(float dt)
{
    if (m_ShakeTimer <= 0.0f) return;

    float rndX = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float rndY = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float rndZ = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    Vector3 offset = Vector3(rndX, rndY, rndZ) * m_ShakeIntensity;
    m_Position += offset;
    m_Target   += offset;

    m_ShakeIntensity *= SHAKE_DAMPING;
    m_ShakeTimer     -= dt;
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
    XMVECTOR vClip    = XMVector3TransformCoord(vPos, viewProj);

    float x = XMVectorGetX(vClip);
    float y = XMVectorGetY(vClip);
    float z = XMVectorGetZ(vClip);

    if (z < 0.0f || z > 1.0f)         return false;
    if (x < -margin || x > margin)    return false;
    if (y < -margin || y > margin)    return false;
    return true;
}
