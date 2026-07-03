#include "main.h"
#include "input.h"
#include "mouse.h"   // ← 追加！
#include "gamepad.h"
#include "inputVisualizer.h"
#include "imgui/imgui.h"

std::vector<InputVisualizer::KeyView> InputVisualizer::m_Keys;
bool InputVisualizer::m_Show = true;

void InputVisualizer::Init()
{
    m_Keys.clear();

    m_Keys.push_back(KeyView("Q", 'Q'));
    m_Keys.push_back(KeyView("W", 'W'));
    m_Keys.push_back(KeyView("E", 'E'));
    m_Keys.push_back(KeyView("R", 'R'));

    m_Keys.push_back(KeyView("A", 'A'));
    m_Keys.push_back(KeyView("S", 'S'));
    m_Keys.push_back(KeyView("D", 'D'));
    m_Keys.push_back(KeyView("F", 'F'));

    m_Keys.push_back(KeyView("UP", VK_UP));
    m_Keys.push_back(KeyView("DOWN", VK_DOWN));
    m_Keys.push_back(KeyView("LEFT", VK_LEFT));
    m_Keys.push_back(KeyView("RIGHT", VK_RIGHT));

    m_Keys.push_back(KeyView("Shift", VK_SHIFT));
    m_Keys.push_back(KeyView("Space", VK_SPACE));
}

void InputVisualizer::Update()
{
    if (!m_Show) return;
    float dt = 1.0f / 60.0f;

    for (auto& key : m_Keys)
    {
        key.Press = Input::GetKeyPress(key.KeyCode);
        if (key.Press) key.PressTimer += dt;
        else           key.PressTimer = 0.0f;
    }
}

void InputVisualizer::DrawKey(const char* name, float width, float height)
{
    bool isPressed = false;
    for (const auto& key : m_Keys) {
        if (key.Name == name) {
            isPressed = key.Press;
            break;
        }
    }

    if (isPressed) {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(30, 144, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(30, 144, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(30, 144, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
    }

    ImGui::Button(name, ImVec2(width, height));

    if (isPressed) ImGui::PopStyleColor(4);
}

void InputVisualizer::DrawPadButton(const char* name, float width, float height, WORD buttonCode)
{
    bool isPressed = Gamepad::GetButtonPress(buttonCode);

    if (isPressed) {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(30, 144, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(30, 144, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(30, 144, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
    }

    ImGui::Button(name, ImVec2(width, height));

    if (isPressed) ImGui::PopStyleColor(4);
}

void InputVisualizer::DrawMouseButton(const char* name, float width, float height, BYTE buttonCode)
{
    bool isPressed = Mouse::GetClickPress(buttonCode);

    if (isPressed) {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(30, 144, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(30, 144, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(30, 144, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
    }

    ImGui::Button(name, ImVec2(width, height));

    if (isPressed) ImGui::PopStyleColor(4);
}

void InputVisualizer::Draw()
{
    if (!m_Show) return;

    if (ImGui::CollapsingHeader("DebugKey"))
    {
        ImGui::Spacing();

        // ==========================================
        // キーボード（左ブロック） + マウス（右ブロック） を横並び
        // ==========================================

        // --- キーボードブロック 開始 ---
        ImGui::BeginGroup();
        {
            ImGui::Text("Keyboard");

            // 1段目 (Q, W, E, R)
            DrawKey("Q", 40.0f, 40.0f); ImGui::SameLine();
            DrawKey("W", 40.0f, 40.0f); ImGui::SameLine();
            DrawKey("E", 40.0f, 40.0f); ImGui::SameLine();
            DrawKey("R", 40.0f, 40.0f);

            // 2段目 (A, S, D, F)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15.0f);
            DrawKey("A", 40.0f, 40.0f); ImGui::SameLine();
            DrawKey("S", 40.0f, 40.0f); ImGui::SameLine();
            DrawKey("D", 40.0f, 40.0f); ImGui::SameLine();
            DrawKey("F", 40.0f, 40.0f);

            // 3段目 (Shift, Space)
            DrawKey("Shift", 60.0f, 40.0f); ImGui::SameLine();
            DrawKey("Space", 120.0f, 40.0f);
        }
        ImGui::EndGroup(); // --- キーボードブロック 終了 ---

        // ==========================================
// 矢印キー
// ==========================================
        ImGui::SameLine(0.0f, 20.0f);

        ImGui::BeginGroup();
        {
            ImGui::Text("Arrow");

            // 上
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 50.0f);
            DrawKey("UP", 50.0f, 40.0f);

            // 左・右
            DrawKey("LEFT", 50.0f, 40.0f);
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 50.0f);
            DrawKey("RIGHT", 50.0f, 40.0f);

            // 下
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 50.0f);
            DrawKey("DOWN", 50.0f, 40.0f);
        }
        ImGui::EndGroup();

        // マウスブロックをキーボードの右に配置
        ImGui::SameLine(0.0f, 20.0f);

        // --- マウスブロック 開始 ---
        ImGui::BeginGroup();
        {
            ImGui::Text("Mouse");

            // 左クリック・右クリックを横並び
            DrawMouseButton("L", 50.0f, 70.0f, VK_LBUTTON); ImGui::SameLine();
            DrawMouseButton("R", 50.0f, 70.0f, VK_RBUTTON);

            // 中クリック（横幅を合わせて中央に）
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 25.0f);
            DrawMouseButton("M", 50.0f, 40.0f, VK_MBUTTON);
        }
        ImGui::EndGroup(); // --- マウスブロック 終了 ---

        ImGui::Spacing();
        ImGui::Separator();

        // ==========================================
        // ゲームパッドの表示
        // ==========================================
        ImGui::Text("Gamepad");
        if (Gamepad::IsConnected())
        {
            ImGui::BeginGroup();
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 45.0f);
                DrawPadButton("UP", 40.0f, 40.0f, XINPUT_GAMEPAD_DPAD_UP);

                DrawPadButton("LEFT", 40.0f, 40.0f, XINPUT_GAMEPAD_DPAD_LEFT); ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 43.0f);
                DrawPadButton("RIGHT", 40.0f, 40.0f, XINPUT_GAMEPAD_DPAD_RIGHT);

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 45.0f);
                DrawPadButton("DOWN", 40.0f, 40.0f, XINPUT_GAMEPAD_DPAD_DOWN);
            }
            ImGui::EndGroup();

            ImGui::SameLine(0.0f, 40.0f);

            ImGui::BeginGroup();
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 45.0f);
                DrawPadButton("Y", 40.0f, 40.0f, XINPUT_GAMEPAD_Y);

                DrawPadButton("X", 40.0f, 40.0f, XINPUT_GAMEPAD_X); ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 43.0f);
                DrawPadButton("B", 40.0f, 40.0f, XINPUT_GAMEPAD_B);

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 45.0f);
                DrawPadButton("A", 40.0f, 40.0f, XINPUT_GAMEPAD_A);
            }
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::Spacing();

            float lx = Gamepad::GetLeftStickX();
            float ly = Gamepad::GetLeftStickY();
            ImGui::SliderFloat("LStick X", &lx, -1.0f, 1.0f);
            ImGui::SliderFloat("LStick Y", &ly, -1.0f, 1.0f);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Not Connected");
        }
    }
}