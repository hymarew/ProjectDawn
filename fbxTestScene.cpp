#include "main.h"
#include "fbxTestScene.h"
#include "sceneManager.h"
#include "renderer.h"
#include "dynamicLightManager.h"
#include "input.h"
#include <vector>

// ワールド座標をスクリーン座標へ投影する(ColliderDebugRendererと同じ考え方)
static bool ProjectToScreen(const XMMATRIX& viewProjection, const Vector3& worldPos, ImVec2& outScreenPos)
{
    XMVECTOR clip = XMVector4Transform(XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f), viewProjection);
    float w = XMVectorGetW(clip);
    if (w <= 0.0f) return false;

    outScreenPos.x = ( XMVectorGetX(clip) / w * 0.5f + 0.5f) * SCREEN_WIDTH;
    outScreenPos.y = (-XMVectorGetY(clip) / w * 0.5f + 0.5f) * SCREEN_HEIGHT;
    return true;
}

// Idle/Run/Jump切り替えボタン用。ラベルと読み込むFBXパスの対応表
struct AnimEntry { const char* Label; const char* FileName; };
static const AnimEntry kAnimEntries[] = {
    { "Idle", "asset\\model\\fbx\\Idle.fbx" },
    { "Run",  "asset\\model\\fbx\\Running.fbx" },
    { "Jump", "asset\\model\\fbx\\Jump.fbx" },
};

void FbxTestScene::Init()
{
    ShowCursor(TRUE);
    m_TestObject.Init();
}

void FbxTestScene::Uninit()
{
    // GameObject::Uninit()がComponent(FbxModelRenderer)のUninit()を呼び、
    // ボーン行列バッファ(m_BoneMatrixBuffer)を解放する。呼ばないとシーンに入り直すたびにリークする。
    m_TestObject.Uninit();
}

void FbxTestScene::Update(float dt)
{
    // ESCでタイトルへ戻る
    if (Input::GetKeyTrigger(VK_ESCAPE))
    {
        g_SceneManager.RequestChange(SceneID::Title);
        return;
    }

    // モデルを全周から確認できるよう、カメラをゆっくり周回させる
    m_OrbitAngle += dt * 0.5f;

    m_TestObject.Update(dt);
}

void FbxTestScene::Draw()
{
    Renderer::Begin();

    // 既存のCameraクラス(Player依存)は使わず、このシーン専用のView/Projectionを組む
    XMVECTOR eye = XMVectorSet(sinf(m_OrbitAngle) * 3.0f, 1.2f, -cosf(m_OrbitAngle) * 3.0f, 0.0f);
    XMVECTOR target = XMVectorSet(0.0f, 0.9f, 0.0f, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(eye, target, up);
    XMMATRIX projection = XMMatrixPerspectiveFovLH(
        1.0f, (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);

    Renderer::SetViewMatrix(view);
    Renderer::SetProjectionMatrix(projection);

    g_DynamicLightManager.Apply();

    if (m_ShowModel)
        m_TestObject.Draw();

    // ---- デバッグUI: モデル本体の表示on/off、ボーン骨格の表示on/off ----
    ImGui::Begin("FbxTestScene Debug");
    ImGui::Text("IsSkinned: %s", m_TestObject.IsSkinned() ? "true" : "false");
    ImGui::Checkbox("Show Model", &m_ShowModel);
    ImGui::Checkbox("Show Bones", &m_ShowBones);
    ImGui::Separator();
    ImGui::Text("Animation:");
    for (int i = 0; i < (int)(sizeof(kAnimEntries) / sizeof(kAnimEntries[0])); i++)
    {
        if (i > 0) ImGui::SameLine();

        bool isCurrent = (i == m_CurrentAnimIndex);
        if (isCurrent) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));

        if (ImGui::Button(kAnimEntries[i].Label))
        {
            m_TestObject.PlayAnimation(kAnimEntries[i].FileName);
            m_CurrentAnimIndex = i;
        }

        if (isCurrent) ImGui::PopStyleColor();
    }
    ImGui::Separator();
    ImGui::Text("Playback: %s", m_TestObject.HasAnimation() ? "playing (loop)" : "stopped");
    if (m_TestObject.HasAnimation())
    {
        ImGui::Text("  \"%s\" %.2fs / %.2fs", m_TestObject.GetCurrentClipName(), m_TestObject.GetAnimTime(), m_TestObject.GetClipDuration());
        ImGui::Text("  Matched bones: %d / %d", m_TestObject.GetMatchedAnimationBoneCount(), m_TestObject.GetSkeletonBoneCount());
    }
    ImGui::Separator();
    ImGui::Text("ESC: Back to Title");
    ImGui::End();

    // ---- ボーン骨格の描画(親子をつなぐ線+関節点) ----
    if (m_ShowBones)
    {
        std::vector<Vector3> bonePositions;
        std::vector<int>     boneParents;
        std::vector<bool>    boneHasOffset;

        if (m_TestObject.GetBoneWorldPositions(bonePositions, boneParents, boneHasOffset))
        {
            XMMATRIX viewProjection = view * projection;
            ImDrawList* dl = ImGui::GetBackgroundDrawList();

            const ImU32 lineColor  = IM_COL32(0, 255, 128, 255);
            const ImU32 jointColor = IM_COL32(255, 64, 64, 255);

            // 親子を結ぶ線
            // ※ 骨格には$AssimpFbx$_Translation/$AssimpFbx$_PreRotationのような、実際のスキニングに
            //   使われない中間ノード(常にバインドポーズ=モデル原点付近に固定)も挟まっている
            //   (例: LeftUpLeg(実ボーン) → $AssimpFbx$_Translation → $AssimpFbx$_PreRotation → LeftLeg(実ボーン))。
            //   直接の親を辿ると中間ノードで途切れる/原点付近へ紛らわしい線が伸びるため、
            //   「実際にスキニングへ使われる直近の祖先ボーン」まで遡って接続する。
            for (size_t i = 0; i < bonePositions.size(); i++)
            {
                if (!boneHasOffset[i]) continue;

                int ancestor = boneParents[i];
                while (ancestor >= 0 && !boneHasOffset[ancestor])
                    ancestor = boneParents[ancestor];

                if (ancestor < 0) continue;

                ImVec2 childScreen, parentScreen;
                bool childOk  = ProjectToScreen(viewProjection, bonePositions[i], childScreen);
                bool parentOk = ProjectToScreen(viewProjection, bonePositions[ancestor], parentScreen);

                if (childOk && parentOk)
                    dl->AddLine(parentScreen, childScreen, lineColor, 2.0f);
            }

            // 関節点(同様に実ボーンのみ)
            for (size_t i = 0; i < bonePositions.size(); i++)
            {
                if (!boneHasOffset[i]) continue;

                ImVec2 screenPos;
                if (ProjectToScreen(viewProjection, bonePositions[i], screenPos))
                    dl->AddCircleFilled(screenPos, 3.0f, jointColor);
            }
        }
    }

    // Phase毎の動作確認用ステータス表示
    {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        char text[128];
        sprintf_s(text, "FbxTestScene | IsSkinned: %s | ESC: Title",
            m_TestObject.IsSkinned() ? "true" : "false");
        dl->AddText(ImVec2(10, 10), IM_COL32(255, 255, 0, 255), text);
    }

    // 他の全シーンと同様、Draw()内でNewFrame()と対にしてImGuiのフレームを閉じる必要がある
    // (main.cppのメインループ側でImGui::NewFrame()のみ毎フレーム呼ばれているため)
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    Renderer::End();
}
