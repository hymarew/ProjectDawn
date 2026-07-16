#pragma once

// =====================================================
// AchievementScreen : 実績一覧のオーバーレイ画面
//
// TitleScene から開き、全実績の名前・条件・解放状態を一覧表示する。
// データは AchievementManager（定義）と SaveManager（解放状態）を
// 参照するだけで、このクラス自身は状態を持たない。
//
// 操作: ESC / Enter で閉じる
// =====================================================
class AchievementScreen
{
public:
    void Open()  { m_IsOpen = true; }

    // 閉じる操作が行われたら false → 閉じて true を返す
    bool Update();

    void Draw();

    bool IsOpen() const { return m_IsOpen; }

private:
    bool m_IsOpen = false;
};
