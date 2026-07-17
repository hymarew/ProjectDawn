#include "main.h"
#include "optionsMenu.h"
#include "input.h"
#include "renderer.h"
#include "saveManager.h"
#include "soundManager.h"
#include "inputManager.h"
#include "GameConfig.h"

#include <algorithm>

using namespace GameConfig::Options;

// C++14 のため std::clamp の代わりに使う
static float Clamp(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// -------------------------------------------------------
// Open : 現在の設定値を読み込んで編集状態を初期化する
// -------------------------------------------------------
void OptionsMenu::Open()
{
    m_IsOpen        = true;
    m_SelectedIndex = 0;

    // 各システムが持つ現在値をそのまま編集値にする
    // （Manager::Init で save.json の値が反映済みのため二重ロード不要）
    m_BgmVolume   = g_SoundManager.GetBgmVolume();
    m_SEVolume    = g_SoundManager.GetSEVolume();
    m_Sensitivity = InputManager::GetMouseSensitivityScale();
}

// -------------------------------------------------------
// Close : 編集値を save.json へ保存して閉じる
// -------------------------------------------------------
void OptionsMenu::Close()
{
    if (!m_IsOpen) return;
    m_IsOpen = false;

    g_SaveManager.SetSettingFloat(KEY_BGM_VOLUME,  m_BgmVolume);
    g_SaveManager.SetSettingFloat(KEY_SE_VOLUME,   m_SEVolume);
    g_SaveManager.SetSettingFloat(KEY_SENSITIVITY, m_Sensitivity);
    g_SaveManager.Save();
}

// -------------------------------------------------------
// AdjustValue : 選択項目の値を1目盛り増減して即時反映する
// -------------------------------------------------------
void OptionsMenu::AdjustValue(int direction)
{
    switch (m_SelectedIndex)
    {
    case ITEM_BGM:
        m_BgmVolume = Clamp(m_BgmVolume + VOLUME_STEP * direction, 0.0f, 1.0f);
        g_SoundManager.SetBgmVolume(m_BgmVolume);
        break;

    case ITEM_SE:
        m_SEVolume = Clamp(m_SEVolume + VOLUME_STEP * direction, 0.0f, 1.0f);
        g_SoundManager.SetSEVolume(m_SEVolume);
        // 変更結果をすぐ確認できるように SE を1回鳴らす
        g_SoundManager.PlaySE(SEType::ShotgunFire);
        break;

    case ITEM_SENSITIVITY:
        m_Sensitivity = Clamp(m_Sensitivity + SENSITIVITY_STEP * direction,
                              SENSITIVITY_MIN, SENSITIVITY_MAX);
        InputManager::SetMouseSensitivityScale(m_Sensitivity);
        break;
    }
}

// -------------------------------------------------------
// Update : 入力処理。閉じたら true を返す
// -------------------------------------------------------
bool OptionsMenu::Update()
{
    if (!m_IsOpen) return false;

    // 項目選択（↑ W / ↓ S）
    if (Input::GetKeyTrigger(VK_UP)   || Input::GetKeyTrigger('W'))
        m_SelectedIndex = (m_SelectedIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
    if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
        m_SelectedIndex = (m_SelectedIndex + 1) % ITEM_COUNT;

    // 値変更（← A / → D）
    if (Input::GetKeyTrigger(VK_LEFT)  || Input::GetKeyTrigger('A'))
        AdjustValue(-1);
    if (Input::GetKeyTrigger(VK_RIGHT) || Input::GetKeyTrigger('D'))
        AdjustValue(+1);

    // 閉じる（ESC はどこでも / Enter は BACK 選択時のみ）
    bool closeReq = Input::GetKeyTrigger(VK_ESCAPE);
    if ((Input::GetKeyTrigger(VK_RETURN) || Input::GetKeyTrigger(VK_SPACE))
        && m_SelectedIndex == ITEM_BACK)
        closeReq = true;

    if (closeReq)
    {
        Close();
        return true;
    }
    return false;
}

// -------------------------------------------------------
// Draw : 半透明オーバーレイ + 設定パネル
// -------------------------------------------------------
void OptionsMenu::Draw()
{
    if (!m_IsOpen) return;

    // PauseMenu と同じく最前面に描画する
    ImDrawList* dl   = ImGui::GetForegroundDrawList();
    ImFont*     font = ImGui::GetFont();

    // ---- 全画面半透明黒オーバーレイ ----
    dl->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT),
        IM_COL32(0, 0, 0, 160));

    // ---- パネル ----
    constexpr float PANEL_W = 520.0f;
    constexpr float PANEL_H = 360.0f;
    const float panelX = (SCREEN_WIDTH  - PANEL_W) * 0.5f;
    const float panelY = (SCREEN_HEIGHT - PANEL_H) * 0.5f;

    dl->AddRectFilled(
        ImVec2(panelX, panelY),
        ImVec2(panelX + PANEL_W, panelY + PANEL_H),
        IM_COL32(10, 10, 20, 220), 8.0f);
    dl->AddRect(
        ImVec2(panelX, panelY),
        ImVec2(panelX + PANEL_W, panelY + PANEL_H),
        IM_COL32(100, 140, 220, 200), 8.0f, 0, 1.5f);

    // ---- タイトル "OPTIONS" ----
    {
        const float sz   = 36.0f;
        const char* text = "OPTIONS";
        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, text);
        float tx  = panelX + (PANEL_W - ts.x) * 0.5f;
        float ty  = panelY + 24.0f;

        dl->AddText(font, sz, ImVec2(tx + 2, ty + 2), IM_COL32(0, 0, 0, 160), text);
        dl->AddText(font, sz, ImVec2(tx, ty), IM_COL32(180, 200, 255, 255), text);
    }

    // 区切り線
    dl->AddLine(
        ImVec2(panelX + 20.0f, panelY + 72.0f),
        ImVec2(panelX + PANEL_W - 20.0f, panelY + 72.0f),
        IM_COL32(80, 100, 180, 160), 1.0f);

    // ---- 設定項目（ラベル + ゲージ + 値） ----
    const float itemStart = panelY + 96.0f;
    const float itemStep  = 62.0f;
    const float labelSz   = 22.0f;

    // ゲージ描画ヘルパー: ratio(0〜1) を横バーで表示する
    auto DrawGauge = [&](float y, float ratio, bool selected)
    {
        const float gx = panelX + 40.0f;
        const float gw = PANEL_W - 180.0f;
        const float gh = 12.0f;

        dl->AddRectFilled(ImVec2(gx, y), ImVec2(gx + gw, y + gh),
            IM_COL32(30, 30, 40, 220), 3.0f);
        if (ratio > 0.0f)
            dl->AddRectFilled(ImVec2(gx + 1, y + 1),
                ImVec2(gx + 1 + (gw - 2) * ratio, y + gh - 1),
                selected ? IM_COL32(120, 180, 255, 240) : IM_COL32(90, 120, 180, 200), 3.0f);
        dl->AddRect(ImVec2(gx, y), ImVec2(gx + gw, y + gh),
            IM_COL32(150, 150, 170, 160), 3.0f, 0, 1.0f);
    };

    struct Row { const char* label; float ratio; char valueText[16]; };
    Row rows[3];
    rows[0].label = "BGM Volume";
    rows[0].ratio = m_BgmVolume;
    snprintf(rows[0].valueText, sizeof(rows[0].valueText), "%d%%", (int)(m_BgmVolume * 100.0f + 0.5f));
    rows[1].label = "SE Volume";
    rows[1].ratio = m_SEVolume;
    snprintf(rows[1].valueText, sizeof(rows[1].valueText), "%d%%", (int)(m_SEVolume * 100.0f + 0.5f));
    rows[2].label = "Mouse Sensitivity";
    rows[2].ratio = (m_Sensitivity - SENSITIVITY_MIN) / (SENSITIVITY_MAX - SENSITIVITY_MIN);
    snprintf(rows[2].valueText, sizeof(rows[2].valueText), "x%.1f", m_Sensitivity);

    for (int i = 0; i < 3; i++)
    {
        const float iy       = itemStart + itemStep * i;
        const bool  selected = (i == m_SelectedIndex);

        // 選択ハイライト背景
        if (selected)
            dl->AddRectFilled(
                ImVec2(panelX + 14.0f, iy - 6.0f),
                ImVec2(panelX + PANEL_W - 14.0f, iy + itemStep - 14.0f),
                IM_COL32(60, 100, 200, 90), 4.0f);

        // カーソル + ラベル
        if (selected)
            dl->AddText(font, labelSz, ImVec2(panelX + 20.0f, iy),
                IM_COL32(120, 180, 255, 255), ">");
        dl->AddText(font, labelSz, ImVec2(panelX + 40.0f, iy),
            selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(170, 170, 190, 210),
            rows[i].label);

        // ゲージ + 値
        DrawGauge(iy + labelSz + 6.0f, rows[i].ratio, selected);
        dl->AddText(font, 20.0f,
            ImVec2(panelX + PANEL_W - 120.0f, iy + labelSz + 1.0f),
            selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 200, 210),
            rows[i].valueText);
    }

    // ---- BACK ----
    {
        const float  by       = itemStart + itemStep * 3 + 4.0f;
        const bool   selected = (m_SelectedIndex == ITEM_BACK);
        const char*  label    = "Back";
        const float  sz       = 24.0f;

        if (selected)
            dl->AddRectFilled(
                ImVec2(panelX + 14.0f, by - 6.0f),
                ImVec2(panelX + PANEL_W - 14.0f, by + sz + 4.0f),
                IM_COL32(60, 100, 200, 120), 4.0f);

        ImVec2 ls = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, label);
        float  lx = panelX + (PANEL_W - ls.x) * 0.5f;
        if (selected)
            dl->AddText(font, sz, ImVec2(lx - 26.0f, by), IM_COL32(120, 180, 255, 255), ">");
        dl->AddText(font, sz, ImVec2(lx, by),
            selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(160, 160, 180, 200), label);
    }

    // ---- 操作ヒント ----
    {
        const char* hint = "W/S : Select    A/D : Change    ESC : Back";
        const float sz   = 15.0f;
        ImVec2 hs = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, hint);
        dl->AddText(font, sz,
            ImVec2(panelX + (PANEL_W - hs.x) * 0.5f, panelY + PANEL_H - 30.0f),
            IM_COL32(140, 140, 160, 200), hint);
    }
}
