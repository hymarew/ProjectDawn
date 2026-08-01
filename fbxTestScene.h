#pragma once
#include <vector>
#include "scene.h"
#include "fbxTestObject.h"
#include "animationClip.h"

// =====================================================
// FbxTestScene : FBX読み込み(Phase1)の動作確認専用のデバッグシーン。
// 既存のCamera/Player/Managerには一切触れず、このシーン単独で
// View/Projection行列を組んでFbxTestObjectを描画するだけの最小構成。
// SceneID::FbxTest としてのみ到達可能(タイトル画面のデバッグキーから遷移)。
// 動作確認が完了したら削除して問題ない。
// =====================================================
class FbxTestScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;

private:
    FbxTestObject m_TestObject;
    float         m_OrbitAngle = 0.0f;

    // ImGuiデバッグ表示切り替え(モデル本体の表示on/off・ボーン骨格の表示on/off)
    bool m_ShowModel = true;
    bool m_ShowBones = false;

    // Idle/Run/Jump切り替え用。押したボタンに応じてFbxTestObject::PlayAnimation()を呼ぶ。
    int m_CurrentAnimIndex = 0;
};
