#include "main.h"
#include "input.h"
#include "mouse.h"
#include "gamepad.h"
#include "inputVisualizer.h"
#include "inputManager.h"
#include "manager.h"

float InputManager::s_MouseSensitivityScale = 1.0f;

void InputManager::Init()
{
    // 現状は特に初期化処理なし
    // （将来的にキーコンフィグを追加する場合ここで設定を読む）
    Input::Init();
    Mouse::Init();
    Gamepad::Init();
    InputVisualizer::Init();
}

void InputManager::Update()
{
    // 各入力クラスのUpdate()はそれぞれのクラスで呼ぶため、ここは空でOK
    // まとめて呼びたい場合はここに Input::Update() 等を移しても良い
    Input::Update();
    Mouse::Update();
    Gamepad::Update();
    InputVisualizer::Update();
}

// ============================================================
// 押し続け判定
// ============================================================
bool InputManager::IsPressed(Action action)
{
    switch (action)
    {
    case Action::MOVE_FORWARD:
        return Input::GetKeyPress('W')
            || Gamepad::GetLeftStickY() > 0.3f;

    case Action::MOVE_BACKWARD:
        return Input::GetKeyPress('S')
            || Gamepad::GetLeftStickY() < -0.3f;

    case Action::MOVE_LEFT:
        return Input::GetKeyPress('A')
            || Gamepad::GetLeftStickX() < -0.3f;

    case Action::MOVE_RIGHT:
        return Input::GetKeyPress('D')
            || Gamepad::GetLeftStickX() > 0.3f;

    case Action::JUMP:
        return Input::GetKeyPress(VK_SPACE)
            || Gamepad::GetButtonPress(XINPUT_GAMEPAD_A);

    case Action::SPRINT:
        return Input::GetKeyPress(VK_SHIFT)
            || Gamepad::GetButtonPress(XINPUT_GAMEPAD_LEFT_THUMB);

    case Action::ATTACK:
        return Mouse::GetClickPress(VK_LBUTTON)
            || Gamepad::GetButtonPress(XINPUT_GAMEPAD_X);

    case Action::RELOAD:
        return Input::GetKeyPress('R')
            || Gamepad::GetButtonPress(XINPUT_GAMEPAD_B);

    default:
        return false;
    }
}

// ============================================================
// 押した瞬間だけ判定
// ============================================================
bool InputManager::IsTriggered(Action action)
{
    switch (action)
    {
    case Action::MOVE_FORWARD:
        return Input::GetKeyTrigger('W');

    case Action::MOVE_BACKWARD:
        return Input::GetKeyTrigger('S');

    case Action::MOVE_LEFT:
        return Input::GetKeyTrigger('A');

    case Action::MOVE_RIGHT:
        return Input::GetKeyTrigger('D');

    case Action::JUMP:
        return Input::GetKeyTrigger(VK_SPACE)
            || Gamepad::GetButtonTrigger(XINPUT_GAMEPAD_A);

    case Action::SPRINT:
        return Input::GetKeyTrigger(VK_SHIFT)
            || Gamepad::GetButtonTrigger(XINPUT_GAMEPAD_LEFT_THUMB);

    case Action::ATTACK:
        return Mouse::GetClickTrigger(VK_LBUTTON)
            || Gamepad::GetButtonTrigger(XINPUT_GAMEPAD_X);

    case Action::RELOAD:
        return Input::GetKeyTrigger('R')
            || Gamepad::GetButtonTrigger(XINPUT_GAMEPAD_B);

    default:
        return false;
    }
}

// ============================================================
// スティック/キーボードを統合した移動量の取得
// ============================================================
float InputManager::GetMoveX()
{
    // スティックが動いていればスティック優先
    float stickX = Gamepad::GetLeftStickX();
    if (stickX != 0.0f) return stickX;

    // キーボードは 0 か 1 の離散値
    if (Input::GetKeyPress('D')) return  1.0f;
    if (Input::GetKeyPress('A')) return -1.0f;
    return 0.0f;
}

float InputManager::GetMoveY()
{
    float stickY = Gamepad::GetLeftStickY();
    if (stickY != 0.0f) return stickY;

    if (Input::GetKeyPress('W')) return  1.0f;
    if (Input::GetKeyPress('S')) return -1.0f;
    return 0.0f;
}

// ============================================================
// スティック/キーボード/マウスを統合した カメラ回転量 の取得
// ============================================================
float InputManager::GetCameraMoveX()
{
    if (g_ShowDebugUI) return 0.0f;

    int mouseX = Mouse::GetDeltaX();

    if (mouseX != 0)
    {
        return mouseX * MOUSE_SENSITIVITY * s_MouseSensitivityScale;
    }

    float stickX = Gamepad::GetRightStickX();

    if (fabsf(stickX) > 0.1f)
    {
        return stickX * STICK_SENSITIVITY;
    }

    if (Input::GetKeyPress(VK_RIGHT))
    {
        return STICK_SENSITIVITY;
    }

    if (Input::GetKeyPress(VK_LEFT))
    {
        return -STICK_SENSITIVITY;
    }

    return 0.0f;
}

float InputManager::GetCameraMoveY()
{
    if (g_ShowDebugUI) return 0.0f;

    int mouseY = Mouse::GetDeltaY();

    if (mouseY != 0)
    {
        return -mouseY * MOUSE_SENSITIVITY * s_MouseSensitivityScale;
    }

    float stickY = Gamepad::GetRightStickY();

    if (fabsf(stickY) > 0.1f)
    {
        return stickY * STICK_SENSITIVITY;
    }

    if (Input::GetKeyPress(VK_UP))
    {
        return STICK_SENSITIVITY;
    }

    if (Input::GetKeyPress(VK_DOWN))
    {
        return -STICK_SENSITIVITY;
    }

    return 0.0f;
}