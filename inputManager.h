#pragma once

// ゲーム内の操作アクション一覧
// 増やしたい操作はここに追加するだけ
enum class Action
{
    MOVE_FORWARD,   // 前進
    MOVE_BACKWARD,  // 後退
    MOVE_LEFT,      // 左移動
    MOVE_RIGHT,     // 右移動
    JUMP,           // ジャンプ
    SPRINT,         // ダッシュ
    ATTACK,         // 攻撃（左クリック or Aボタン）
    RELOAD,         // リロード（R キー or B ボタン）
};

// ============================================================
// 入力管理クラス
// キーボード・マウス・ゲームパッドを統合して問い合わせできる
// ============================================================
class InputManager
{

private:
    static constexpr float MOUSE_SENSITIVITY = 0.002f;
    static constexpr float STICK_SENSITIVITY = 0.03f;

    // オプション設定によるマウス感度倍率（MOUSE_SENSITIVITY に乗算する）
    static float s_MouseSensitivityScale;

public:
    static void Init();
    static void Update();

    // 押し続けているか（WASDの移動など）
    static bool IsPressed(Action action);

    // 押した瞬間だけか（ジャンプ・攻撃など）
    static bool IsTriggered(Action action);

    // 移動方向の強さを取得 (-1.0f ～ 1.0f)
    // キーボードなら 0 か 1、スティックなら中間値も返る
    static float GetMoveX();  // 左右 (左: 負, 右: 正)
    static float GetMoveY();  // 前後 (後: 負, 前: 正)

    // 追加：カメラの回転量を取得 (-1.0f ～ 1.0f)
    static float GetCameraMoveX(); // 視点左右 (左: 負, 右: 正)
    static float GetCameraMoveY(); // 視点上下 (下: 負, 上: 正)

    // マウス感度倍率（オプション設定）。OptionsMenu と Manager::Init から呼ばれる
    static void  SetMouseSensitivityScale(float scale) { s_MouseSensitivityScale = scale; }
    static float GetMouseSensitivityScale()            { return s_MouseSensitivityScale; }
};