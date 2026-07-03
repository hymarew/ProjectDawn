
#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "keyboard.h"
#include "debugInfo.h"
#include "mouse.h"
#include <chrono>
#include <thread>

const char* CLASS_NAME = "AppClass";
const char* WINDOW_NAME = "AlienShooter";


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam
);

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

constexpr int MAX_UPDATE = 5;

HWND g_Window;

HWND GetWindow()
{
	return g_Window;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{


	WNDCLASSEX wcex;
	{
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = 0;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = nullptr;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = CLASS_NAME;
		wcex.hIconSm = nullptr;

		RegisterClassEx(&wcex);


		RECT rc = { 0, 0, (LONG)SCREEN_WIDTH, (LONG)SCREEN_HEIGHT };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

		g_Window = CreateWindowEx(0, CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
	}

	CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);


	Manager::Init();
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(g_Window);

	ImGui_ImplDX11_Init(
		Renderer::GetDevice(),
		Renderer::GetDeviceContext()
	);

	InitDebugInfo();
	ShowWindow(g_Window, nCmdShow);
	UpdateWindow(g_Window);
	ShowCursor(FALSE); // 起動時はカーソルを非表示（1キーで表示切替）


	using clock = std::chrono::high_resolution_clock;
	auto oldTime = clock::now();
	const float FIXED_DT = 1.0f / 60.0f;
	float accumulator = 0.0f;

	//DWORD dwExecLastTime;
	//DWORD dwCurrentTime;
	//timeBeginPeriod(1);
	//dwExecLastTime = timeGetTime();
	//dwCurrentTime = 0;



	MSG msg;
	while(1)
	{
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				break;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
        }
		else
		{
			//--------------------------------
			// 経過時間計測
			//--------------------------------
			auto currentTime = clock::now();

			float frameTime =
				std::chrono::duration<float>(
					currentTime - oldTime
				).count();
			g_DebugInfo.frameTime = frameTime;

			oldTime = currentTime;

			//--------------------------------
			// dt暴走防止
			//--------------------------------
			if (frameTime > 0.1f)
			{
				frameTime = 0.1f;
			}

			accumulator += frameTime * g_DebugInfo.timeScale;
			g_DebugInfo.accumulator = accumulator;
			//--------------------------------
			// 固定60FPS更新
			//--------------------------------
			int updateLoop = 0;

			while (accumulator >= FIXED_DT &&
				updateLoop < MAX_UPDATE)
			{
				AddUpdateCount();

				Manager::Update(FIXED_DT);

				accumulator -= FIXED_DT;

				updateLoop++;
			}
			if (updateLoop >= MAX_UPDATE)
			{
				g_DebugInfo.maxUpdateReached++;

				accumulator = 0.0f;
			}

			//--------------------------------
			// 描画
			//--------------------------------
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			AddDrawCount();

			UpdateDebugInfo();

			Manager::Draw();

			Renderer::End();
		}
		//{
		//	dwCurrentTime = timeGetTime();
		//	if((dwCurrentTime - dwExecLastTime) >= (1000 / 60))
		//	{
		//		dwExecLastTime = dwCurrentTime;
		//		UpdateFPS(g_Window);
		//		ImGui_ImplDX11_NewFrame();
		//		ImGui_ImplWin32_NewFrame();
		//		ImGui::NewFrame();
		//		Manager::Update();
		//		Manager::Draw();
		//		Renderer::End();
		//	}
		//}
	}

	timeEndPeriod(1);

	UnregisterClass(CLASS_NAME, wcex.hInstance);

	Manager::Uninit();
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();


	CoUninitialize();

	return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;
	// キーボード入力更新
	switch (uMsg)
	{
	case WM_ACTIVATEAPP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		Keyboard_ProcessMessage(uMsg, wParam, lParam);
		break;
	}

	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_MOUSEWHEEL:
		// HIWORD(wParam): 正 = 手前に回した(上), 負 = 奥に回した(下)
		Mouse::AddScrollDelta((short)HIWORD(wParam));
		break;

	case WM_KEYDOWN:
		// ESC はゲーム側でポーズとして処理するため、ここでは何もしない
		// ウィンドウを閉じるには Alt+F4 を使う
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}