#include "tutorialOverlay.h"

void TutorialOverlay::Draw()
{
    if (!m_IsActive) return;

    // ForegroundDrawList を使うことでクロスヘア・ダメージ表示より前面に描画される
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // ---- 全画面半透明オーバーレイ ----
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(0, 0, 0, 153)); // alpha 153 ≈ 0.6

    // ---- 中央パネル ----
    const float PANEL_W = 460.0f;
    const float PANEL_H = 420.0f;
    const float px = (SCREEN_WIDTH  - PANEL_W) * 0.5f;
    const float py = (SCREEN_HEIGHT - PANEL_H) * 0.5f;

    // パネル背景
    dl->AddRectFilled(
        ImVec2(px, py), ImVec2(px + PANEL_W, py + PANEL_H),
        IM_COL32(15, 15, 25, 230), 6.0f);

    // パネル枠線
    dl->AddRect(
        ImVec2(px, py), ImVec2(px + PANEL_W, py + PANEL_H),
        IM_COL32(120, 160, 220, 200), 6.0f, 0, 1.5f);

    // ---- ヘッダー区切り線 ----
    const float lineY = py + 80.0f;
    dl->AddLine(
        ImVec2(px + 20.0f, lineY), ImVec2(px + PANEL_W - 20.0f, lineY),
        IM_COL32(80, 120, 180, 160), 1.0f);

    // ---- フッター区切り線 ----
    const float footerY = py + PANEL_H - 52.0f;
    dl->AddLine(
        ImVec2(px + 20.0f, footerY), ImVec2(px + PANEL_W - 20.0f, footerY),
        IM_COL32(80, 120, 180, 160), 1.0f);

    ImFont* font = ImGui::GetFont();

    // タイトルを中央揃えで描画するヘルパー
    auto DrawCentered = [&](float y, float fontSize, ImU32 color, const char* text)
    {
        ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
        float tx = px + (PANEL_W - ts.x) * 0.5f;
        // 影
        dl->AddText(font, fontSize, ImVec2(tx + 1.5f, y + 1.5f),
            IM_COL32(0, 0, 0, 160), text);
        dl->AddText(font, fontSize, ImVec2(tx, y), color, text);
    };

    // 左揃えで描画するヘルパー（キー説明用）
    auto DrawLeft = [&](float x, float y, float fontSize, ImU32 color, const char* text)
    {
        dl->AddText(font, fontSize, ImVec2(x + 1.0f, y + 1.0f),
            IM_COL32(0, 0, 0, 140), text);
        dl->AddText(font, fontSize, ImVec2(x, y), color, text);
    };

    // ---- タイトル ----
    DrawCentered(py + 18.0f, 28.0f, IM_COL32(220, 220, 255, 255), "ALIEN SHOOTER");
    DrawCentered(py + 52.0f, 16.0f, IM_COL32(160, 180, 220, 200), "CONTROLS");

    // ---- 操作説明（キー : アクション） ----
    const float keyX    = px + 50.0f;
    const float descX   = px + 210.0f;
    const float startY  = lineY + 16.0f;
    const float lineH   = 26.0f;
    const float keySize = 18.0f;

    ImU32 keyCol  = IM_COL32(255, 220, 100, 255); // キー名: 黄
    ImU32 descCol = IM_COL32(210, 210, 210, 240); // 説明:   白

    struct Entry { const char* key; const char* desc; };
    Entry entries[] = {
        { "WASD",         ": Move"            },
        { "Mouse",        ": Camera"          },
        { "Left Click",   ": Fire"            },
        { "Right Click",  ": Aim"             },
        { "Scroll Wheel", ": Switch Weapon"   },
        { "R",            ": Reload"          },
        { "Shift",        ": Sprint"          },
        { "Space",        ": Jump"            },
        { "ESC",          ": Pause"           },
    };

    for (int i = 0; i < (int)(sizeof(entries) / sizeof(entries[0])); i++)
    {
        float y = startY + i * lineH;
        DrawLeft(keyX,  y, keySize, keyCol,  entries[i].key);
        DrawLeft(descX, y, keySize, descCol, entries[i].desc);
    }

    // ---- ゴール説明 ----
    const float goalY = footerY - 42.0f;
    DrawCentered(goalY,        14.0f, IM_COL32(140, 210, 140, 220), "Destroy all Spawners and survive every Wave!");

    // ---- "Press Any Key" 点滅 ----
    // ImGui の経過時間で点滅させる
    float t     = ImGui::GetTime();
    float blink = (sinf((float)t * 3.5f) * 0.5f + 0.5f); // 0.0 〜 1.0
    int   alpha = 140 + (int)(blink * 115.0f);            // 140 〜 255

    DrawCentered(py + PANEL_H - 36.0f, 18.0f,
        IM_COL32(180, 200, 255, alpha), "Press Any Key to Start");
}
