#pragma once

// =====================================================
// OptionsMenu : オプション設定のオーバーレイメニュー
//
// BGM音量 / SE音量 / マウス感度 をゲージ表示で調整する。
// 値は変更した瞬間に SoundManager / InputManager へ反映され、
// メニューを閉じるときに save.json へまとめて保存される。
//
// PauseMenu（ゲーム中）と TitleScene（タイトル）の両方から
// 開けるように、シーンに依存しない独立クラスにしている。
//
// 操作: W/S(↑↓) 項目選択、A/D(←→) 値変更、Enter/ESC で閉じる
// =====================================================
class OptionsMenu
{
public:
    void Open();   // 開くときに呼ぶ（現在の設定値を読み込む）
    void Close();  // 閉じるときに呼ぶ（save.json へ保存する）

    // 入力処理。閉じる操作が行われたら Close() を呼び true を返す
    bool Update();

    void Draw();   // 半透明オーバーレイ + 設定パネル描画

    bool IsOpen() const { return m_IsOpen; }

private:
    // メニュー項目。BACK は「閉じる」ボタンとして最後に置く
    enum Item
    {
        ITEM_BGM = 0,
        ITEM_SE,
        ITEM_SENSITIVITY,
        ITEM_BACK,
        ITEM_COUNT,
    };

    // 選択中の項目の値を direction(-1/+1) 方向に1目盛り動かして即時反映する
    void AdjustValue(int direction);

    bool  m_IsOpen        = false;
    int   m_SelectedIndex = 0;

    // 編集中の値（Open で読み込み、変更のたびに各システムへ即時反映する）
    float m_BgmVolume   = 0.8f;
    float m_SEVolume    = 0.8f;
    float m_Sensitivity = 1.0f;
};
