#pragma once
#include "iTransition.h"
#include <memory>
#include <unordered_map>

// =====================================================
// TransitionManager : 現在再生中の Transition のみを保持するクラス
//
// 使い方の詳細・各トランジションのパラメータ一覧は transitionType.h の
// コメントにまとめてあるので、まずそちらを参照。
//
//   g_TransitionManager.Play(TransitionType::Fade, TransitionMode::Out);
//
// 各 TransitionType のインスタンスは Init() で1度だけ生成・初期化し、
// レジストリ(m_Registry)で保持し続ける（GPUリソースを毎回作り直さないため）。
// m_Current はその中のどれが「今再生中か」を指すだけの観測用ポインタで、
// 所有権は持たない。「Managerは現在再生中のTransitionのみ保持する」という
// 要件は、この m_Current 1つだけが実際の再生状態を表す、という形で満たしている。
//
// 新しいトランジションを追加する手順（switch文は一切増えない）:
//   1. ITransition/TransitionBase を継承した新クラスを作る (例: WipeTransition)
//   2. Init() 内に Register<WipeTransition>(TransitionType::Wipe); を1行追加する
// =====================================================
class TransitionManager
{
public:
    void Init();   // 全トランジション種別を生成・登録する
    void Uninit();

    // duration <= 0 を渡すとそのトランジションのデフォルト時間が使われる
    void Play(TransitionType type, TransitionMode mode, float duration = -1.0f);
    void Stop();

    void Update(float dt);
    void Draw();

    bool IsPlaying()  const { return m_Current && m_Current->IsPlaying();  }
    bool IsFinished() const { return m_Current && m_Current->IsFinished(); }

    // 色や強さ等を変更したい場合、Play() の前にこれで実体を取得して Set*** を呼ぶ。
    // 例: g_TransitionManager.GetTransition<FadeTransition>(TransitionType::Fade)->SetColor(...);
    // 未登録の type や型が一致しない場合は nullptr を返す。
    template <typename T>
    T* GetTransition(TransitionType type)
    {
        auto it = m_Registry.find(type);
        if (it == m_Registry.end()) return nullptr;
        return static_cast<T*>(it->second.get());
    }

private:
    // T を type として登録する（T は ITransition 派生クラス）
    template <typename T>
    void Register(TransitionType type)
    {
        auto transition = std::make_unique<T>();
        transition->Init();
        m_Registry[type] = std::move(transition);
    }

    std::unordered_map<TransitionType, std::unique_ptr<ITransition>> m_Registry;
    ITransition* m_Current = nullptr; // 現在再生中のTransition（所有はしない）
};

extern TransitionManager g_TransitionManager;
