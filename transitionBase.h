#pragma once
#include "iTransition.h"
#include "easing.h"

// =====================================================
// TransitionBase : 全トランジション共通処理を集約する Template Method 基底クラス
//
// 責務（ここで完結させる）:
//   - timer / duration 管理
//   - progress(0→1) 計算
//   - イージング適用
//   - 終了判定（IsFinished は完了した1フレームのみ true）
//
// 派生クラスが書くのはそのトランジション固有の処理だけでよい:
//   - Init()   : GPUリソースの用意（シェーダー・頂点バッファ・テクスチャ等）
//   - Uninit() : GPUリソースの解放
//   - Draw()   : GetEasedProgress() / GetMode() を使って実際に描画する
// =====================================================
class TransitionBase : public ITransition
{
public:
    // Start/Update は共通処理として固定する（Template Method）。
    // 派生クラスで上書きさせたくないため final にしている。
    void Start(TransitionMode mode, float duration) final;
    void Update(float dt) final;

    bool IsPlaying()  const override { return m_Playing;  }
    bool IsFinished() const override { return m_Finished; }

    // 再生中は常に true。終了後は Out なら覆った状態を維持するため true、
    // In なら完全に元へ戻っているので false になる（以後 Draw() は呼ばれない）
    bool IsVisible() const override { return m_Playing || m_Mode == TransitionMode::Out; }

    // Init() / Uninit() / Draw() はトランジション固有処理のため
    // ITransition のまま純粋仮想（派生クラスに実装を強制する）

protected:
    // defaultDuration : Play() で duration を省略/0以下指定した場合に使われる値
    // easing          : このトランジションが使うイージング関数
    explicit TransitionBase(float defaultDuration = 0.5f, EasingType easing = EasingType::Linear)
        : m_DefaultDuration(defaultDuration), m_Easing(easing)
    {
    }

    // イージング適用後の進行度（0=開始直後 → 1=終了）
    // 派生クラスはこれと GetMode() だけを使って描画すればよい
    float GetEasedProgress() const { return Easing::Apply(m_Easing, m_Progress); }

    TransitionMode GetMode() const { return m_Mode; }

private:
    // ---- 派生クラス側では変更しない設定値 ----
    float      m_DefaultDuration;
    EasingType m_Easing;

    // ---- 再生状態 ----
    TransitionMode m_Mode     = TransitionMode::In;
    float          m_Timer    = 0.0f;
    float          m_Duration = 0.5f;
    float          m_Progress = 0.0f; // 0→1（イージング適用前の生の時間進行度）
    bool           m_Playing  = false;
    bool           m_Finished = false;
};
