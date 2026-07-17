#pragma once

// =====================================================
// CreditScreen : 権利表記（クレジット）のオーバーレイ画面
//
// TitleScene から開き、使用素材の提供元を表示する。
//   BGM : 魔王魂 (MaouDamashii)      https://maou.audio/
//   SE  : 効果音ラボ (SoundEffect-Lab) https://soundeffect-lab.info/
// ※ ImGui 標準フォントは日本語非対応のため、画面上はローマ字+URLで表記する
//
// 操作: ESC / Enter で閉じる
// =====================================================
class CreditScreen
{
public:
    void Open()  { m_IsOpen = true; }

    // 閉じる操作が行われたら閉じて true を返す
    bool Update();

    void Draw();

    bool IsOpen() const { return m_IsOpen; }

private:
    bool m_IsOpen = false;
};
