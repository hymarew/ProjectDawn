//システム系
#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "input.h"
#include "inputManager.h"
#include "inputVisualizer.h"
#include "mouse.h"
#include "debugInfo.h"

//オブジェクト系
#include "gameObject.h"
#include "camera.h"
#include "field.h"
#include "player.h"

bool g_ShowDebugUI = false;

std::list<GameObject*> Manager::m_GameObject;

void Manager::Init()
{
	Renderer::Init();
	InputManager::Init();

	// PlayerのUpdate後にCameraが追従できるよう、Playerを先に登録する
	AddGameObject<Player>();
	AddGameObject<Camera>();
	AddGameObject<Field>();
}

void Manager::Uninit()
{
	for (GameObject* gameObject : m_GameObject)
	{
		gameObject->Uninit();
		delete gameObject;
	}
	m_GameObject.clear();

	Mouse::Uninit();
	Renderer::Uninit();
}

void Manager::Update(float dt)
{
	InputManager::Update();

	// 1キーでデバッグUIとカーソル表示を切り替える
	if (Input::GetKeyTrigger('1'))
	{
		g_ShowDebugUI = !g_ShowDebugUI;
		ShowCursor(g_ShowDebugUI);
		Mouse::SetLocked(!g_ShowDebugUI);
	}

	for (GameObject* gameObject : m_GameObject)
	{
		gameObject->Update(dt);
	}
}

void Manager::Draw()
{
	Renderer::Begin();

	// カメラのView/Projectionを先に確定させてから他のオブジェクトを描画する
	Camera* camera = GetGameObject<Camera>();
	if (camera) camera->Draw();

	for (GameObject* gameObject : m_GameObject)
	{
		if (gameObject == camera) continue;
		gameObject->Draw();
	}

	ImGuiDraw();
}

void Manager::ImGuiDraw()
{
	if (g_ShowDebugUI)
	{
		ImGui::Begin("Debug");

		if (ImGui::CollapsingHeader("DebugInfo"))
		{
			DebugSystemInfo();
		}

		InputVisualizer::Draw();

		ImGui::End();
	}

	// クロスヘア（画面中央に固定）
	{
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		float cx = SCREEN_WIDTH  * 0.5f;
		float cy = SCREEN_HEIGHT * 0.5f;
		const float len = 10.0f;
		const float gap =  3.0f;
		ImU32 col = IM_COL32(255, 255, 255, 220);
		dl->AddLine(ImVec2(cx - len - gap, cy), ImVec2(cx - gap, cy), col, 1.5f);
		dl->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + len + gap, cy), col, 1.5f);
		dl->AddLine(ImVec2(cx, cy - len - gap), ImVec2(cx, cy - gap), col, 1.5f);
		dl->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + len + gap), col, 1.5f);
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Manager::DebugSystemInfo()
{
	ImGui::SeparatorText("System");

	ImGui::Text("FPS : %.1f", g_DebugInfo.FPS);

	ImGui::SliderFloat("Time Scale", &g_DebugInfo.timeScale, 0.1f, 10.0f);

	ImGui::Text("Frame Time : %.4f ms", g_DebugInfo.frameTime * 1000.0f);

	ImGui::Text("Accumulator : %.4f", g_DebugInfo.accumulator);

	ImGui::Text("Update Count : %d", g_DebugInfo.updateCount);

	ImGui::Text("Draw Count : %d", g_DebugInfo.drawCount);

	ImGui::Text("MaxUpdate Reached : %d", g_DebugInfo.maxUpdateReached);
}
