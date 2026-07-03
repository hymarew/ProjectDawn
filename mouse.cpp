#include "main.h"
#include "mouse.h"
#include "manager.h"

BYTE Mouse::m_OldKeyState[256];
BYTE Mouse::m_KeyState[256];
int  Mouse::m_X           = 0;
int  Mouse::m_Y           = 0;
int  Mouse::m_DeltaX      = 0;
int  Mouse::m_DeltaY      = 0;
bool Mouse::m_Locked      = false;
int  Mouse::m_ScrollDelta    = 0;
int  Mouse::m_ScrollDeltaRaw = 0;

void Mouse::Init()
{
    memset(m_OldKeyState, 0, 256);
    memset(m_KeyState, 0, 256);
    m_X = 0;
    m_Y = 0;
}

void Mouse::Uninit()
{
}

void Mouse::Update()
{
    // WndProc が蓄積した Raw 値を確定値へ転送してからリセット。
    // PeekMessage → Mouse::Update の順序でも値が消えない。
    m_ScrollDelta    = m_ScrollDeltaRaw;
    m_ScrollDeltaRaw = 0;
    memcpy(m_OldKeyState, m_KeyState, 256);
    GetKeyboardState(m_KeyState);

    POINT pt;
    GetCursorPos(&pt);

    static POINT oldPt = pt;

    m_DeltaX = pt.x - oldPt.x;
    m_DeltaY = pt.y - oldPt.y;

    // ゲームプレイ中（m_Locked=true かつデバッグUI非表示）はカーソルを中央固定
    if (m_Locked && !g_ShowDebugUI)
    {
        HWND hWnd = GetWindow();
        RECT rc;
        GetClientRect(hWnd, &rc);
        POINT center = { (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
        ClientToScreen(hWnd, &center);
        SetCursorPos(center.x, center.y);
        oldPt = center;
    }
    else
    {
        oldPt = pt;
    }

    ScreenToClient(GetWindow(), &pt);

    m_X = pt.x;
    m_Y = pt.y;
}

bool Mouse::GetClickPress(BYTE KeyCode)
{
    return (m_KeyState[KeyCode] & 0x80);
}

bool Mouse::GetClickTrigger(BYTE KeyCode)
{
    return ((m_KeyState[KeyCode] & 0x80) && !(m_OldKeyState[KeyCode] & 0x80));
}