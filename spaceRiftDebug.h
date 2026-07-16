#pragma once
#include "spaceRift.h"
#include "bloomPass.h"

// =====================================================
// SpaceRiftDebugPanel : SpaceRiftデバッグデモの統括
//
// Spawn状態の保持・ImGuiからの操作・Bloom合成込みの描画をまとめて担当する。
// 以前はManagerが直接SpaceRift/BloomPassの内部（Material等）を触っていたが、
// Managerの責務ではないためこのクラスへ分離した。
// Manager側は各タイミング（Init/Uninit/Update/ImGuiDraw）で呼ぶだけでよい。
//
// SpaceRift本体はデバッグ専用ではないので、将来ゲーム内イベントへ
// 組み込む場合はこのクラスの使い方（BeginEmissivePass→Draw→Composite）を
// 参考にSpaceRift+BloomPassを直接使えばよい。
// =====================================================
class SpaceRiftDebugPanel
{
public:
    void Init();           // Manager::Init から呼ぶ（BloomPassのGPUリソース確保）
    void Uninit();         // Manager::Uninit から呼ぶ
    void Update(float dt); // Spawn済みの間だけ時間経過・パーティクル放出を進める

    // Bloom合成込みの描画。gameSceneのDraw（パーティクル描画後・HUD前）から呼ぶ。
    // 未Spawn時は何もしない。呼び出し時点でシーンカメラのView/Projectionが設定済みであること
    void Draw();

    // "Space Rift" デバッグパネル。Manager::ImGuiDraw から呼ぶ
    void DrawImGui();

private:
    SpaceRift m_Rift;
    BloomPass m_Bloom;
    bool      m_Active = false; // Spawn済みかどうか（未Spawn時はUpdate/Drawしない）
};

// グローバルインスタンス（spaceRiftDebug.cpp で定義）
extern SpaceRiftDebugPanel g_SpaceRiftDebug;
