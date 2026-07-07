#pragma once

class Player;

// =====================================================
// WeaponUI : 画面右下に武器情報を描画するクラス
//
// 責務:
//   - 武器名・残弾数・リロードゲージを ImGui BackgroundDrawList で描画する
//   - Weapon クラスに描画責務を持たせず、UI を完全に分離する
//
// 表示内容:
//   ASSAULT RIFLE
//   25 / 30        ← 残弾 20% 以下で黄色、0 で赤
//   [████████░░]   ← リロード中のみ表示（青緑のゲージ）
//   Reloading...   ← リロード中のみ表示
//
// 将来拡張:
//   - 複数武器のスロット表示（武器切替 UI）
//   - 予備弾薬（Reserve : 180）
//   - 武器アイコン画像
//   - クールダウンゲージ
//   - 武器レベル表示
// =====================================================
class WeaponUI
{
public:
    // Player から現在の武器情報を取得して描画する
    // GameScene::Draw() の ImGui::Render() 前に呼ぶこと
    void Draw(const Player* player);

private:
    // レイアウト定数（解像度変更に対応しやすいよう定数化）
    static constexpr float PANEL_W   = 210.0f;  // パネル幅
    static constexpr float PANEL_H   = 110.0f;  // パネル高さ（リロードゲージなし時）
    static constexpr float MARGIN_R  =  30.0f;  // 画面右端からの余白
    static constexpr float MARGIN_B  =  80.0f;  // 画面下端からの余白（クロスヘアと被らない位置）
    static constexpr float GAUGE_H   =  12.0f;  // リロードゲージの高さ
    static constexpr float WARN_RATIO = 0.20f;  // 残弾警告しきい値（20%）
};
