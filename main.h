#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define NOMINMAX
#include <windows.h>
#include <assert.h>
#include <functional>
#include <list>
#include <vector>
#include <Xinput.h>


#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")


#include <DirectXMath.h>
using namespace DirectX;

#ifdef _DEBUG
#pragma comment(lib, "DirectXTex_Debug.lib")
#else
#pragma comment(lib, "DirectXTex_Release.lib")
#endif


#pragma comment(lib, "xinput.lib")

#pragma comment (lib, "winmm.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"


//#define SCREEN_WIDTH	(1280)
//#define SCREEN_HEIGHT	(720)

#define SCREEN_WIDTH  (1600)
#define SCREEN_HEIGHT (900)

//#define SCREEN_WIDTH  (1920)
//#define SCREEN_HEIGHT (1080)


HWND GetWindow();

void Invoke(std::function<void()> Function, int Time);

