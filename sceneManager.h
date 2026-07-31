#pragma once
#include <memory>
#include "scene.h"
#include "transitionType.h"

// =====================================================
// SceneID : 遷移先を指定するID
// =====================================================
enum class SceneID
{
    Title,         // タイトル画面（起動の入口）
    MainMenu,      // 拠点メニュー（出撃・武器変更・EXTRA・設定へのハブ）
    WeaponSelect,  // 武器変更画面（装備を変更して拠点へ戻る）
    StageSelect,   // ステージ選択画面（出撃導線: Story モード専用）
    Extra,         // 高難易度コンテンツ選択画面（Endless 等。ストーリークリアで解放）
    Game,          // ゲーム本編
    Result,        // リザルト画面
    StoryComplete, // ストーリー全クリア画面（最終ステージクリア時）
};

// =====================================================
// SceneManager : 現在のシーンを管理するクラス
//
// 使い方:
//   g_SceneManager.RequestChange(SceneID::Game); // どこからでも呼べる
// =====================================================
class SceneManager
{
public:
    // 最初のシーン(TitleScene)を生成する
    void Init();

    // 現在のシーンを解放する
    void Uninit();

    // 毎フレーム: 遷移チェック → 現在シーンのUpdate
    void Update(float dt);

    // 毎フレーム: 現在シーンのDraw
    void Draw();

    // シーン遷移リクエスト（次のUpdateの先頭で切り替わる）
    void RequestChange(SceneID id);

private:
    std::unique_ptr<Scene> m_CurrentScene;       // 現在動いているシーン
    SceneID                m_NextSceneID;         // 遷移先ID
    bool                   m_HasRequest = false;  // 遷移リクエストがあるか

    // シーン切り替え1回分でOut/Inに同じ種類を使うため、RequestChange時に選んで保持しておく
    TransitionType m_TransitionType = TransitionType::Fade;

    // 登録されている全トランジション種別からランダムに1つ選ぶ
    static TransitionType PickRandomTransitionType();

    // 実際にシーンを切り替える（Updateの先頭で呼ばれる）
    void ApplyChange();

    // IDに対応するシーンをnewして返す
    std::unique_ptr<Scene> CreateScene(SceneID id);
};

// グローバルインスタンス（sceneManager.cppで定義）
extern SceneManager g_SceneManager;
