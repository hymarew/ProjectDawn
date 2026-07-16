#include "main.h"
#include "sceneManager.h"
#include "transitionManager.h"
#include "titleScene.h"
#include <cstdlib>
#include "menuScene.h"
#include "weaponSelectScene.h"
#include "stageSelectScene.h"
#include "storyCompleteScene.h"
#include "gameScene.h"
#include "resultScene.h"
#include "input.h"
#include "mouse.h"
#include "soundManager.h"

// グローバルインスタンス（extern 宣言は sceneManager.h にある）
SceneManager g_SceneManager;

// ---------------------------------------------------------
// SceneIDToBgmType : シーン種別に応じたBGMを決める
// ゲーム本編(Game)のみ専用BGM、それ以外(タイトル・メニュー・武器選択・
// ステージ選択・リザルト等)は共通のMenu BGMを使う
// ---------------------------------------------------------
static BgmType SceneIDToBgmType(SceneID id)
{
    return (id == SceneID::Game) ? BgmType::Game : BgmType::Menu;
}

// ---------------------------------------------------------
// PickRandomTransitionType : 登録されている全種別からランダムに1つ選ぶ
// 新しいTransitionTypeを追加したらここにも1行足すこと
// ---------------------------------------------------------
TransitionType SceneManager::PickRandomTransitionType()
{
    static const TransitionType kTypes[] = {
        TransitionType::Fade,
        TransitionType::Wipe,
        TransitionType::Circle,
        TransitionType::Slide,
        TransitionType::Curtain,
        TransitionType::Mosaic,
        TransitionType::Blur,
        TransitionType::Distortion,
        TransitionType::PixelDissolve,
    };
    constexpr int kCount = sizeof(kTypes) / sizeof(kTypes[0]);
    return kTypes[rand() % kCount];
}

// ---------------------------------------------------------
// Init : タイトルシーンからスタート、FadeIn で登場
// ---------------------------------------------------------
void SceneManager::Init()
{
    m_CurrentScene = std::make_unique<TitleScene>();
    m_CurrentScene->Init();

    // 起動時はタイトルがフェードインして登場する（起動時だけは固定のFadeにしておく）
    g_TransitionManager.Play(TransitionType::Fade, TransitionMode::In);

    g_SoundManager.PlayBgm(SceneIDToBgmType(SceneID::Title));
}

// ---------------------------------------------------------
// Uninit : 現在のシーンを解放する
// ---------------------------------------------------------
void SceneManager::Uninit()
{
    if (m_CurrentScene)
    {
        m_CurrentScene->Uninit();
        m_CurrentScene.reset();
    }
}

// ---------------------------------------------------------
// Update : FadeOut 完了でシーン切り替え → FadeIn 開始
// ---------------------------------------------------------
void SceneManager::Update(float dt)
{
    g_TransitionManager.Update(dt);

    // Out が完了した瞬間にシーンを切り替えて In を開始する（Out と同じ種類を使う）
    if (m_HasRequest && g_TransitionManager.IsFinished())
    {
        ApplyChange();
        m_HasRequest = false;
        g_TransitionManager.Play(m_TransitionType, TransitionMode::In);
    }

    if (m_CurrentScene)
        m_CurrentScene->Update(dt);
}

// ---------------------------------------------------------
// Draw : 現在シーンの描画
// ---------------------------------------------------------
void SceneManager::Draw()
{
    if (m_CurrentScene)
        m_CurrentScene->Draw();
}

// ---------------------------------------------------------
// RequestChange : ランダムなトランジションのOutを起動してから次フレームに切り替える
// ---------------------------------------------------------
void SceneManager::RequestChange(SceneID id)
{
    if (m_HasRequest) return;  // 二重リクエスト防止

    m_NextSceneID     = id;
    m_HasRequest       = true;
    m_TransitionType   = PickRandomTransitionType(); // Out/Inで同じ種類を使う
    g_TransitionManager.Play(m_TransitionType, TransitionMode::Out);
}

// ---------------------------------------------------------
// ApplyChange : 旧シーンを Uninit して新シーンを Init する
// ---------------------------------------------------------
void SceneManager::ApplyChange()
{
    if (m_CurrentScene)
    {
        m_CurrentScene->Uninit();
        m_CurrentScene.reset();
    }

    // シーン切り替え時に前シーンのキー入力を引き継がないようリセットする
    Input::Init();
    Mouse::Init();

    // GetAsyncKeyState の内部ラッチビット（0x0001）を全キー分消費してクリアする。
    // Input::Init() では m_KeyState しかリセットできず、
    // Windows カーネルが管理するラッチは別途消費しないと残り続ける。
    for (int vk = 0; vk < 256; vk++) GetAsyncKeyState(vk);

    m_CurrentScene = CreateScene(m_NextSceneID);
    m_CurrentScene->Init();

    // シーン切り替えに合わせてBGMを切り替える（同じ種類が続く場合は継続再生される）
    g_SoundManager.PlayBgm(SceneIDToBgmType(m_NextSceneID));
}

// ---------------------------------------------------------
// CreateScene : SceneID に対応するシーンを生成して返す
// ---------------------------------------------------------
std::unique_ptr<Scene> SceneManager::CreateScene(SceneID id)
{
    switch (id)
    {
    case SceneID::Title:        return std::make_unique<TitleScene>();
    case SceneID::Menu:         return std::make_unique<MenuScene>();
    case SceneID::WeaponSelect: return std::make_unique<WeaponSelectScene>();
    case SceneID::StageSelect:   return std::make_unique<StageSelectScene>();
    case SceneID::Game:          return std::make_unique<GameScene>();
    case SceneID::Result:        return std::make_unique<ResultScene>();
    case SceneID::StoryComplete: return std::make_unique<StoryCompleteScene>();
    default:              return std::make_unique<TitleScene>();
    }
}
