#pragma once
#include "vector3.h"

class GameObject;

// =====================================================
// CameraContext : Camera → Strategy への入力データ
// =====================================================
struct CameraContext
{
    Vector3     playerPos;
    float       yaw         = 0.0f;
    float       pitch       = 0.0f;
    float       dt          = 0.0f;
    GameObject* eventTarget = nullptr;
};

// =====================================================
// CameraOutput : Strategy → Camera への出力データ
// =====================================================
struct CameraOutput
{
    Vector3 position;
    Vector3 target;
    float   fov = -1.0f;  // -1 = Camera のデフォルト FOV を使う
};

// =====================================================
// CameraStrategy : 全ストラテジーの基底クラス
// =====================================================
class CameraStrategy
{
public:
    virtual ~CameraStrategy() = default;

    // モード開始・終了時に1回呼ばれる
    // camPos / target には切り替え直前のカメラ状態を渡す（Lerp初期化用）
    virtual void OnEnter(const Vector3& camPos, const Vector3& target) {}
    virtual void OnExit() {}

    // 毎フレーム呼ばれる。position と target を計算して返す
    virtual CameraOutput Update(const CameraContext& ctx) = 0;
};
