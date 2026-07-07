#pragma once
#include <memory>
#include "vector3.h"
#include "gameObject.h"
#include "cameraStrategy.h"

enum class CameraMode
{
    DEFAULT,
    FPS,
    TPS,
    EVENT,
};

class Camera : public GameObject
{
private:
    Vector3 m_Target = {};
    static Camera* m_MainCamera;

    XMMATRIX m_View, m_Projection;

    CameraMode m_Mode = CameraMode::DEFAULT;

    std::unique_ptr<CameraStrategy> m_Strategy;
    GameObject* m_EventTarget = nullptr;

    // ズーム
    float m_CurrentFov    = 1.222f;  // 70°
    float m_ZoomTargetFov = 1.222f;
    float m_ZoomDuration  = 0.0f;
    float m_ZoomTimer     = 0.0f;

    // シェイク
    float m_ShakeTimer     = 0.0f;
    float m_ShakeIntensity = 0.0f;

    void UpdateZoom(float dt);
    void ApplyShake();

public:
    void Init()    override;
    void Uninit()  override;
    void Update(float dt) override;
    void Draw()    override;
    const char* GetName() override { return "Camera"; }

    // モード切り替え（Strategyを差し替える）
    CameraMode GetMode() const { return m_Mode; }
    void SetMode(CameraMode mode);

    // イベントカメラ用ターゲット
    void        SetEventTarget(GameObject* obj) { m_EventTarget = obj; }
    GameObject* GetEventTarget() const          { return m_EventTarget; }

    // ズーム
    void ZoomTo(float targetFov, float duration);

    // シェイク
    void Shake(float duration, float intensity);

    // 視線方向（クロスヘアと一致）
    Vector3 GetAimDirection() const
    {
        Vector3 dir = { m_Target.x - m_Position.x,
                        m_Target.y - m_Position.y,
                        m_Target.z - m_Position.z };
        dir.normalize();
        return dir;
    }

    Vector3 GetRight() override
    {
        Vector3 forward = GetForward();
        Vector3 up      = Vector3(0.0f, 1.0f, 0.0f);
        Vector3 right   = Vector3::corss(up, forward);
        right.normalize();
        return right;
    }

    Vector3 GetUp() override
    {
        Vector3 forward = GetForward();
        Vector3 right   = GetRight();
        Vector3 up      = Vector3::corss(forward, right);
        up.normalize();
        return up;
    }

    Vector3 GetForward() override
    {
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(
            m_Rotation.x, m_Rotation.y, m_Rotation.z);
        Vector3 forward;
        XMStoreFloat3((XMFLOAT3*)&forward, rot.r[2]);
        forward.normalize();
        return forward;
    }

    XMMATRIX GetViewMatrix()       { return m_View; }
    XMMATRIX GetProjectionMatrix() { return m_Projection; }

    bool CheckInView(Vector3 pos, float margin = 1.2f);
};
