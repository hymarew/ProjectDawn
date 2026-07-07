#pragma once

// =====================================================
// FadeState : フェードの状態
// =====================================================
enum class FadeState
{
    None,     // フェードなし（通常描画）
    FadeOut,  // 透明 → 黒 へ変化中
    FadeIn,   // 黒 → 透明 へ変化中
};

// =====================================================
// FadeManager : 画面全体のフェードイン/アウトを管理する
//
// 責務:
//   - Alpha 値を時間経過で線形補間する
//   - 画面全体を覆う黒い矩形を ImGui BackgroundDrawList で描画する
//   - SceneManager が FadeOut 完了を検知してシーン切り替えを実行する
//
// 遷移フロー（SceneManager が制御する）:
//   RequestChange()
//   → StartFadeOut()
//   → IsFinished() == true (FadeOut完了)
//   → ApplyChange() (シーン切り替え)
//   → StartFadeIn()
//   → FadeIn 完了でフェード終了
//
// 将来拡張:
//   - 白フェード（m_Color を可変にする）
//   - イーズイン/アウト曲線
//   - 円形ワイプなど派生エフェクト
// =====================================================
class FadeManager
{
public:
    // フェード時間のデフォルト値（秒）
    static constexpr float DEFAULT_FADE_OUT = 0.5f;
    static constexpr float DEFAULT_FADE_IN  = 0.5f;

    // フェードアウト開始（透明 → 黒）
    void StartFadeOut(float duration = DEFAULT_FADE_OUT);

    // フェードイン開始（黒 → 透明）
    void StartFadeIn(float duration = DEFAULT_FADE_IN);

    // 毎フレーム呼ぶ（Alpha 値を更新する）
    void Update(float dt);

    // ImGui::Render() の直前に呼ぶ（黒矩形を描画する）
    void Draw();

    // このフレームでフェードが完了したか（1フレームだけ true）
    bool IsFinished() const { return m_Finished; }

    // フェード中か
    bool IsFading() const { return m_State != FadeState::None; }

    FadeState GetState() const { return m_State; }

private:
    FadeState m_State    = FadeState::None;
    float     m_Alpha    = 0.0f;   // 0.0:透明 〜 1.0:黒
    float     m_Duration = 0.5f;
    float     m_Timer    = 0.0f;
    bool      m_Finished = false;  // 完了フラグ（1フレームのみ true）
};

extern FadeManager g_FadeManager;
