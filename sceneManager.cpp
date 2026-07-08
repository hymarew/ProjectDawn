#include "main.h"
#include "sceneManager.h"
#include "transitionManager.h"
#include "titleScene.h"
#include "menuScene.h"
#include "stageSelectScene.h"
#include "storyCompleteScene.h"
#include "gameScene.h"
#include "resultScene.h"
#include "input.h"
#include "mouse.h"

// グローバルインスタンス（extern 宣言は sceneManager.h にある）
SceneManager g_SceneManager;

// ---------------------------------------------------------
// Init : タイトルシーンからスタート、FadeIn で登場
// ---------------------------------------------------------
void SceneManager::Init()
{
    m_CurrentScene = std::make_unique<TitleScene>();
    m_CurrentScene->Init();

    // 起動時はタイトルがフェードインして登場する
    g_TransitionManager.Play(TransitionType::Fade, TransitionMode::In);
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

    // FadeOut が完了した瞬間にシーンを切り替えて FadeIn を開始する
    if (m_HasRequest && g_TransitionManager.IsFinished())
    {
        ApplyChange();
        m_HasRequest = false;
        g_TransitionManager.Play(TransitionType::Fade, TransitionMode::In);
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
// RequestChange : FadeOut を起動してから次フレームに切り替える
// ---------------------------------------------------------
void SceneManager::RequestChange(SceneID id)
{
    if (m_HasRequest) return;  // 二重リクエスト防止

    m_NextSceneID = id;
    m_HasRequest  = true;
    g_TransitionManager.Play(TransitionType::Fade, TransitionMode::Out);
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
}

// ---------------------------------------------------------
// CreateScene : SceneID に対応するシーンを生成して返す
// ---------------------------------------------------------
std::unique_ptr<Scene> SceneManager::CreateScene(SceneID id)
{
    switch (id)
    {
    case SceneID::Title:       return std::make_unique<TitleScene>();
    case SceneID::Menu:        return std::make_unique<MenuScene>();
    case SceneID::StageSelect:   return std::make_unique<StageSelectScene>();
    case SceneID::Game:          return std::make_unique<GameScene>();
    case SceneID::Result:        return std::make_unique<ResultScene>();
    case SceneID::StoryComplete: return std::make_unique<StoryCompleteScene>();
    default:              return std::make_unique<TitleScene>();
    }
}
