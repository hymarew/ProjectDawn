#include "main.h"
#include "gamepad.h"

// ============================================================
// 静的メンバ変数の定義
// ============================================================
XINPUT_STATE Gamepad::m_OldState = {};  // 前フレームの状態（初期値: ゼロ）
XINPUT_STATE Gamepad::m_State = {};  // 現在フレームの状態（初期値: ゼロ）
bool         Gamepad::m_Connected = false;  // 起動時は未接続扱い


// ============================================================
// 初期化処理
// Manager や main の Init() から呼ぶ
// ============================================================
void Gamepad::Init()
{
    ZeroMemory(&m_OldState, sizeof(XINPUT_STATE));
    ZeroMemory(&m_State, sizeof(XINPUT_STATE));
    m_Connected = false;
}


// ============================================================
// 終了処理
// 現状は特に解放するリソースはなし
// ============================================================
void Gamepad::Uninit()
{
}


// ============================================================
// 毎フレームの更新処理
// Manager や main の Update() から呼ぶ
// 前フレームの状態を保存してから、最新の入力状態を取得する
// ============================================================
void Gamepad::Update()
{
    // 現在の状態を「前フレームの状態」として保存
    m_OldState = m_State;

    // 取得前にゼロクリア
    ZeroMemory(&m_State, sizeof(XINPUT_STATE));

    // コントローラー 0番 の状態を取得
    // ERROR_SUCCESS が返れば接続されている
    DWORD result = XInputGetState(0, &m_State);
    m_Connected = (result == ERROR_SUCCESS);
}


// ============================================================
// ボタンを押し続けているか判定
// 引数: XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_START など
// ============================================================
bool Gamepad::GetButtonPress(WORD Button)
{
    // 未接続なら常に false を返す
    if (!m_Connected) return false;

    // 指定ボタンのビットが立っていれば true
    return (m_State.Gamepad.wButtons & Button) != 0;
}


// ============================================================
// ボタンを押した瞬間だけ true を返す判定
// 「前フレームは押されていない」かつ「今フレームは押されている」 場合のみ true
// ============================================================
bool Gamepad::GetButtonTrigger(WORD Button)
{
    if (!m_Connected) return false;

    bool now = (m_State.Gamepad.wButtons & Button) != 0;  // 今フレームの状態
    bool old = (m_OldState.Gamepad.wButtons & Button) != 0;  // 前フレームの状態

    return now && !old;  // 今は押されていて、前フレームは押されていない → 押した瞬間
}


// ============================================================
// スティックの正規化ヘルパー（このファイル内でのみ使用）
// raw      : スティックの生の値 (-32768 ～ 32767)
// deadZone : デッドゾーンの閾値（この範囲内は 0 として扱う）
// 戻り値   : -1.0f ～ 1.0f に正規化した値
// ============================================================
static float Normalize(SHORT raw, SHORT deadZone)
{
    // デッドゾーン内なら 0 を返す（スティックの微小なズレを無視）
    if (abs(raw) < deadZone) return 0.0f;

    // -32767 ～ 32767 の範囲を -1.0f ～ 1.0f に変換
    return raw / 32767.0f;
}


// ============================================================
// 左スティック 左右方向の取得
// 左: 負の値, 右: 正の値
// ============================================================
float Gamepad::GetLeftStickX()
{
    return Normalize(m_State.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
}


// ============================================================
// 左スティック 上下方向の取得
// 下: 負の値, 上: 正の値
// ============================================================
float Gamepad::GetLeftStickY()
{
    return Normalize(m_State.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
}


// ============================================================
// 右スティック 左右方向の取得
// ============================================================
float Gamepad::GetRightStickX()
{
    return Normalize(m_State.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
}


// ============================================================
// 右スティック 上下方向の取得
// ============================================================
float Gamepad::GetRightStickY()
{
    return Normalize(m_State.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
}


// ============================================================
// コントローラーが接続されているか確認
// ============================================================
bool Gamepad::IsConnected()
{
    return m_Connected;
}