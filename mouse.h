#pragma once
#include <windows.h>

// ============================================================
// マウス入力管理クラス
// ============================================================
class Mouse
{
private:
    static BYTE m_OldKeyState[256];
    static BYTE m_KeyState[256];

    static int  m_X;
    static int  m_Y;
    static int  m_DeltaX;
    static int  m_DeltaY;
    static bool m_Locked; // true = ゲームプレイ中（カーソルを中央固定）

public:
    static void Init();
    static void Uninit();
    static void Update();

    // ゲームシーンで true、タイトル/リザルトで false にする
    static void SetLocked(bool locked) { m_Locked = locked; }

    // マウスクリック判定 (VK_LBUTTON, VK_RBUTTON, VK_MBUTTON)
    static bool GetClickPress(BYTE KeyCode);
    static bool GetClickTrigger(BYTE KeyCode);

    // マウスカーソルの座標を取得する
    static int GetX() { return m_X; }
    static int GetY() { return m_Y; }
    static int GetDeltaX() { return m_DeltaX; }
    static int GetDeltaY() { return m_DeltaY; }

    // スクロールホイール
    // WndProc の WM_MOUSEWHEEL から AddScrollDelta() で蓄積し、
    // Mouse::Update() で確定値へ転送してから Raw をリセットする。
    // これにより PeekMessage → Update の順序で値が消えるバグを防ぐ。
    static void AddScrollDelta(int delta) { m_ScrollDeltaRaw += delta; }
    static bool GetScrollUp()   { return m_ScrollDelta > 0; }  // ホイールを奥に回した
    static bool GetScrollDown() { return m_ScrollDelta < 0; }  // ホイールを手前に回した

private:
    static int m_ScrollDelta;    // Update 後に Player が読む確定値
    static int m_ScrollDeltaRaw; // WndProc から蓄積中の値（Update でフラッシュ）
};