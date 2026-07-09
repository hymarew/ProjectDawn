#pragma once
#include "transitionType.h"

// =====================================================
// ITransition : 全トランジションが実装するインターフェース（Strategy）
//
// TransitionManager はこの型のポインタしか知らない。
// 「どのトランジションか」を知る必要がなく、種類が増えても
// TransitionManager 自体には一切手を入れなくてよい。
// =====================================================
class ITransition
{
public:
    virtual ~ITransition() = default;

    // GPUリソースなど、アプリ生存中に1度だけ行う初期化・解放
    virtual void Init()   = 0;
    virtual void Uninit() = 0;

    // 再生開始。duration <= 0 の場合はトランジション自身のデフォルト値を使う
    virtual void Start(TransitionMode mode, float duration) = 0;

    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;

    virtual bool IsPlaying()  const = 0;
    virtual bool IsFinished() const = 0; // 完了した"その1フレームだけ" true

    // 今このフレームで Draw() を呼ぶべきか。
    // 再生中は常に true。再生終了後は Out なら覆った状態を維持するため true のまま、
    // In なら完全に元へ戻っているので false（＝以後 Draw() を呼ばない）になる。
    // これが無いと Mosaic/Blur/Distortion 等（アルファ0で自動的に描画をやめる仕組みが無い
    // シェーダー直描画系）が In 完了後も永遠に画面を覆い続けてしまう。
    virtual bool IsVisible() const = 0;
};
