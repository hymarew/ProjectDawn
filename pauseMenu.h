#pragma once

// =====================================================
// PauseMenu : ポーズ中のオーバーレイメニュー
//
// GameScene が所有し、m_IsPaused == true のときだけ
// Update / Draw を呼ぶ。
//
// 選択結果は Get* で取り出し、GameScene 側で処理する。
// =====================================================
class PauseMenu
{
public:
    void Open();   // ポーズ開始時に呼ぶ（選択カーソルをリセット）
    void Close();  // ポーズ解除時に呼ぶ

    void Update(); // ↑↓ 選択・Enter 決定の入力処理
    void Draw();   // 半透明オーバーレイ + メニュー描画

    // GameScene が毎フレーム確認して処理する
    bool IsResumeRequested() const { return m_ResumeReq; }
    bool IsRetryRequested()  const { return m_RetryReq;  }
    bool IsTitleRequested()  const { return m_TitleReq;  }
    bool IsExitRequested()   const { return m_ExitReq;   }

private:
    int  m_SelectedIndex = 0;
    bool m_ResumeReq     = false;
    bool m_RetryReq      = false;
    bool m_TitleReq      = false;
    bool m_ExitReq       = false;

    static constexpr int ITEM_COUNT = 4; // Resume / Retry / Title / Exit
};
