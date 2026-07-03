//システム系
#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "keyboard.h"
#include "mouse.h"
#include "debugInfo.h"

bool g_ShowDebugUI = false;

void Manager::Init()
{
	Renderer::Init();
	Keyboard_Initialize();
	Mouse::Init();
}

void Manager::Uninit()
{
	Mouse::Uninit();
	Renderer::Uninit();
}

void Manager::Update(float dt)
{
	keycopy();
	Mouse::Update();

	// 1キーでデバッグUIとカーソル表示を切り替える
	if (Keyboard_IsKeyDownTrigger(KK_D1))
	{
		g_ShowDebugUI = !g_ShowDebugUI;
		ShowCursor(g_ShowDebugUI);
		Mouse::SetLocked(!g_ShowDebugUI);
	}
}

void Manager::Draw()
{
	Renderer::Begin();

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
