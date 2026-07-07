#pragma once
#include "main.h"

// =====================================================
// TutorialOverlay
// ゲーム開始直後に1回だけ表示する操作説明オーバーレイ。
// 表示中は GameScene 側でゲーム更新を停止する。
// =====================================================
class TutorialOverlay
{
public:
    bool IsActive() const { return m_IsActive; }
    void Close()          { m_IsActive = false; }
    void Draw();

private:
    bool m_IsActive = true;
};
