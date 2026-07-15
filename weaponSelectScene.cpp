#include "main.h"
#include "weaponSelectScene.h"
#include "sceneManager.h"
#include "transitionManager.h"
#include "gameContext.h"
#include "renderer.h"
#include "input.h"
#include <cstdio>
#include <algorithm>

void WeaponSelectScene::Init()
{
    m_SelectedIndex = 0;
    ShowCursor(TRUE);
}

void WeaponSelectScene::Uninit()
{
}

// ---------------------------------------------------------
// BuildEntries : Inventory + WeaponDatabase + EquipLoadout から
//                一覧表示用のデータを組み立てる
// ---------------------------------------------------------
std::vector<WeaponSelectScene::Entry> WeaponSelectScene::BuildEntries() const
{
    std::vector<Entry> entries;

    auto& ctx = GameContext::Instance();
    for (const Inventory::OwnedWeapon* owned : ctx.inventory.GetOwnedWeapons())
    {
        const WeaponData* data = ctx.weaponDB.Find(owned->weaponID);
        if (!data) continue;  // マスタ欠落（Inventory::Init で警告済み）

        Entry entry;
        entry.data  = data;
        entry.isNew = owned->isNew;

        EquipSlot slot = ctx.equipLoadout.FindSlotOf(owned->weaponID);
        entry.equippedSlot = (slot == EquipSlot::Count) ? -1 : (int)slot;

        entries.push_back(entry);
    }
    return entries;
}

// ---------------------------------------------------------
// Update : カーソル移動・装備変更・決定
// ---------------------------------------------------------
void WeaponSelectScene::Update(float dt)
{
    auto& ctx = GameContext::Instance();

    std::vector<Entry> entries = BuildEntries();

    if (!entries.empty())
    {
        // カーソル移動（↑ W / ↓ S）
        int count = (int)entries.size();
        if (Input::GetKeyTrigger(VK_UP)   || Input::GetKeyTrigger('W'))
            m_SelectedIndex = (m_SelectedIndex - 1 + count) % count;
        if (Input::GetKeyTrigger(VK_DOWN) || Input::GetKeyTrigger('S'))
            m_SelectedIndex = (m_SelectedIndex + 1) % count;
        m_SelectedIndex = std::min(m_SelectedIndex, count - 1);

        const Entry& selected = entries[m_SelectedIndex];

        // カーソルを合わせた武器は確認済みとして NEW 表示を消す
        ctx.inventory.ClearNewFlag(selected.data->id);

        // 装備変更（1 = Primary / 2 = Secondary）。EquipLoadout が即セーブする
        if (Input::GetKeyTrigger('1'))
            ctx.equipLoadout.Equip(EquipSlot::Primary, selected.data->id);
        if (Input::GetKeyTrigger('2'))
            ctx.equipLoadout.Equip(EquipSlot::Secondary, selected.data->id);
    }

    // 決定: Primary が装備されていることを確認してから次のシーンへ。
    // Story はステージ選択を経由し、Endless は直接ゲームへ進む
    if (Input::GetKeyTrigger(VK_RETURN) && ctx.equipLoadout.HasPrimary())
    {
        if (ctx.currentMode == GameMode::Story)
            g_SceneManager.RequestChange(SceneID::StageSelect);
        else
            g_SceneManager.RequestChange(SceneID::Game);
    }

    // ESC でモード選択へ戻る
    if (Input::GetKeyTrigger(VK_ESCAPE))
        g_SceneManager.RequestChange(SceneID::Menu);
}

// ---------------------------------------------------------
// Draw : 武器一覧テーブルの描画
// ---------------------------------------------------------
void WeaponSelectScene::Draw()
{
    Renderer::Begin();

    ImDrawList* dl   = ImGui::GetBackgroundDrawList();
    ImFont*     font = ImGui::GetFont();

    // ---- 背景 ----
    dl->AddRectFilled(ImVec2(0.0f, 0.0f),
        ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT), IM_COL32(8, 8, 14, 255));

    std::vector<Entry> entries = BuildEntries();

    const float rowH    = 46.0f;
    const float headerH = 110.0f;
    const float footerH = 48.0f;
    const int   rows    = std::min((int)entries.size(), VISIBLE_ROWS);
    const float panelH  = headerH + rowH * std::max(rows, 1) + footerH;
    const float panelX  = (SCREEN_WIDTH  - PANEL_W) * 0.5f;
    const float panelY  = (SCREEN_HEIGHT - panelH) * 0.5f;

    // ---- パネル ----
    dl->AddRectFilled(ImVec2(panelX, panelY),
        ImVec2(panelX + PANEL_W, panelY + panelH), IM_COL32(15, 15, 25, 235), 8.0f);
    dl->AddRect(ImVec2(panelX, panelY),
        ImVec2(panelX + PANEL_W, panelY + panelH), IM_COL32(100, 140, 220, 200), 8.0f, 0, 1.5f);

    // ---- タイトル ----
    {
        const char* title = "SELECT YOUR WEAPONS";
        const float sz = 32.0f;
        ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, title);
        dl->AddText(font, sz, ImVec2(panelX + (PANEL_W - ts.x) * 0.5f, panelY + 20.0f),
            IM_COL32(180, 200, 255, 255), title);
    }

    // ---- 列見出し ----
    const float colName   = panelX + 24.0f;   // NEW + 名前 + レアリティ
    const float colCat    = panelX + 320.0f;  // カテゴリ
    const float colDmg    = panelX + 420.0f;  // ダメージ
    const float colRate   = panelX + 500.0f;  // 発射間隔（秒）
    const float colReload = panelX + 590.0f;  // リロード時間（秒）
    const float colMag    = panelX + 690.0f;  // 装弾数
    const float colEq     = panelX + 770.0f;  // 装備状態
    {
        const float sz = 13.0f;
        const float hy = panelY + 70.0f;
        ImU32 col = IM_COL32(150, 150, 170, 220);
        dl->AddText(font, sz, ImVec2(colName,   hy), col, "NAME / RARITY");
        dl->AddText(font, sz, ImVec2(colCat,    hy), col, "CATEGORY");
        dl->AddText(font, sz, ImVec2(colDmg,    hy), col, "DMG");
        dl->AddText(font, sz, ImVec2(colRate,   hy), col, "INTERVAL");
        dl->AddText(font, sz, ImVec2(colReload, hy), col, "RELOAD");
        dl->AddText(font, sz, ImVec2(colMag,    hy), col, "MAG");
        dl->AddText(font, sz, ImVec2(colEq,     hy), col, "EQUIP");
        dl->AddLine(ImVec2(panelX + 16.0f, hy + 20.0f),
                    ImVec2(panelX + PANEL_W - 16.0f, hy + 20.0f),
                    IM_COL32(80, 100, 180, 160), 1.0f);
    }

    if (entries.empty())
    {
        dl->AddText(font, 20.0f, ImVec2(colName, panelY + headerH + 8.0f),
            IM_COL32(160, 160, 180, 220), "No weapons.");
    }

    // ---- 一覧（選択行が常に見えるようスクロールする） ----
    int scrollTop = 0;
    if ((int)entries.size() > VISIBLE_ROWS)
    {
        scrollTop = m_SelectedIndex - VISIBLE_ROWS / 2;
        scrollTop = std::max(0, std::min(scrollTop, (int)entries.size() - VISIBLE_ROWS));
    }

    for (int row = 0; row < rows; row++)
    {
        int i = scrollTop + row;
        const Entry& entry = entries[i];
        const WeaponData& data = *entry.data;

        const float y = panelY + headerH + rowH * row;
        const bool selected = (i == m_SelectedIndex);

        // 選択ハイライト背景
        if (selected)
            dl->AddRectFilled(ImVec2(panelX + 12.0f, y - 4.0f),
                ImVec2(panelX + PANEL_W - 12.0f, y + rowH - 8.0f),
                IM_COL32(60, 100, 200, 110), 4.0f);

        RarityColor rc = GetRarityColor(data.rarity);
        ImU32 rarityCol = IM_COL32((int)(rc.r * 255), (int)(rc.g * 255), (int)(rc.b * 255), 255);

        // 名前（レアリティ色）+ NEW バッジ
        dl->AddText(font, 20.0f, ImVec2(colName, y), rarityCol, data.name.c_str());
        if (entry.isNew)
        {
            float nameW = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, data.name.c_str()).x;
            dl->AddText(font, 13.0f, ImVec2(colName + nameW + 8.0f, y + 3.0f),
                IM_COL32(255, 230, 80, 255), "NEW");
        }
        // レアリティ名（小・下段）
        dl->AddText(font, 12.0f, ImVec2(colName, y + 22.0f),
            IM_COL32((int)(rc.r * 255), (int)(rc.g * 255), (int)(rc.b * 255), 190),
            RarityToString(data.rarity));

        // 性能値
        char buf[32];
        ImU32 statCol = IM_COL32(210, 210, 220, 230);
        dl->AddText(font, 16.0f, ImVec2(colCat, y + 6.0f), statCol,
            (data.category == WeaponCategory::AssaultRifle) ? "Assault" : "Rocket");
        snprintf(buf, sizeof(buf), "%.0f", data.damage);
        dl->AddText(font, 17.0f, ImVec2(colDmg, y + 6.0f), statCol, buf);
        snprintf(buf, sizeof(buf), "%.2fs", data.fireRate);
        dl->AddText(font, 17.0f, ImVec2(colRate, y + 6.0f), statCol, buf);
        snprintf(buf, sizeof(buf), "%.1fs", data.reloadTime);
        dl->AddText(font, 17.0f, ImVec2(colReload, y + 6.0f), statCol, buf);
        if (data.magazineSize == -1) snprintf(buf, sizeof(buf), "INF");
        else                         snprintf(buf, sizeof(buf), "%d", data.magazineSize);
        dl->AddText(font, 17.0f, ImVec2(colMag, y + 6.0f), statCol, buf);

        // 装備状態
        if (entry.equippedSlot == (int)EquipSlot::Primary)
            dl->AddText(font, 17.0f, ImVec2(colEq, y + 6.0f), IM_COL32(120, 220, 120, 255), "[P]");
        else if (entry.equippedSlot == (int)EquipSlot::Secondary)
            dl->AddText(font, 17.0f, ImVec2(colEq, y + 6.0f), IM_COL32(120, 180, 255, 255), "[S]");
    }

    // ---- フッター（操作ガイド） ----
    dl->AddText(font, 15.0f,
        ImVec2(panelX + 24.0f, panelY + panelH - 32.0f),
        IM_COL32(150, 150, 170, 220),
        "W/S : Select    1 : Equip Primary    2 : Equip Secondary    Enter : Start    ESC : Back");

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_TransitionManager.Draw();

    Renderer::End();
}
