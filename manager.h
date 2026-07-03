#pragma once

class Manager
{
public:
	static void Init();
	static void Uninit();
	static void Update(float dt);
	static void Draw();
	static void ImGuiDraw();
	static void DebugSystemInfo();
};

extern bool g_ShowDebugUI;
