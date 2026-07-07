#pragma once
#include <vector>
#include <string>
#include <windows.h>

// ============================================================
// 入力可視化クラス (デバッグ用)
// ============================================================
class InputVisualizer
{
private:
    struct KeyView
    {
        std::string Name;
        int KeyCode;
        bool Press;
        float PressTimer;

        KeyView(std::string name, int code)
            : Name(name), KeyCode(code), Press(false), PressTimer(0.0f) {
        }
    };

    static std::vector<KeyView> m_Keys;
    static bool m_Show;

    // キーボード用
    static void DrawKey(const char* name, float width, float height);
    // ゲームパッドボタン用
    static void DrawPadButton(const char* name, float width, float height, WORD buttonCode);
    // マウスボタン用（追加）
    static void DrawMouseButton(const char* name, float width, float height, BYTE buttonCode);

public:
    static void Init();
    static void Update();
    static void Draw();

    static void ToggleShow() { m_Show = !m_Show; }
    static bool IsShow() { return m_Show; }
};