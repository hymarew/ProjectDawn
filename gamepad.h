#pragma once

// ============================================================
// Gamepad クラス
// XInput を使ってゲームパッド（Xbox系コントローラー）の
// 入力状態を管理するクラス。
// Input クラスと同じ使い方ができるよう静的メソッドで設計。
// ============================================================

class Gamepad
{
private:
    static XINPUT_STATE m_OldState;  // 前フレームのコントローラー状態
    static XINPUT_STATE m_State;     // 現在フレームのコントローラー状態
    static bool         m_Connected; // コントローラーが接続されているか

public:
    static void Init();    // 初期化（アプリ起動時に一度だけ呼ぶ）
    static void Uninit();  // 終了処理
    static void Update();  // 毎フレーム呼んで入力状態を更新する

    // ----------------------------------------
    // ボタン判定
    // 引数には XINPUT_GAMEPAD_A など XINPUT の定数を渡す
    // ----------------------------------------
    static bool GetButtonPress(WORD Button);  // 押し続けている間 true
    static bool GetButtonTrigger(WORD Button);  // 押した瞬間だけ true

    // ----------------------------------------
    // スティック入力取得
    // 戻り値は -1.0f ～ 1.0f に正規化済み
    // デッドゾーン内の微小な傾きは 0.0f として扱う
    // ----------------------------------------
    static float GetLeftStickX();   // 左スティック 左右（左: 負, 右: 正）
    static float GetLeftStickY();   // 左スティック 上下（下: 負, 上: 正）
    static float GetRightStickX();  // 右スティック 左右
    static float GetRightStickY();  // 右スティック 上下

    // ----------------------------------------
    // 接続状態の確認
    // ----------------------------------------
    static bool IsConnected();  // コントローラーが接続されていれば true
};