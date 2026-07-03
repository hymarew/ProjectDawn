#include "DebugInfo.h"

#include <windows.h>

DebugInfo g_DebugInfo;

//--------------------------------
// FPS計測用
//--------------------------------
static LARGE_INTEGER frequency;
static LARGE_INTEGER oldTime;

static int drawCount = 0;
static int updateCount = 0;

//--------------------------------
// 初期化
//--------------------------------
void InitDebugInfo()
{
	QueryPerformanceFrequency(&frequency);

	QueryPerformanceCounter(&oldTime);
}

//--------------------------------
// Update回数加算
//--------------------------------
void AddUpdateCount()
{
	updateCount++;
}

//--------------------------------
// Draw回数加算
//--------------------------------
void AddDrawCount()
{
	drawCount++;
}

//--------------------------------
// Debug情報更新
//--------------------------------
void UpdateDebugInfo()
{
	LARGE_INTEGER currentTime;

	QueryPerformanceCounter(&currentTime);

	double elapsed =
		(double)(currentTime.QuadPart - oldTime.QuadPart)
		/ frequency.QuadPart;

	if (elapsed >= 1.0)
	{
		g_DebugInfo.FPS =
			drawCount / (float)elapsed;

		g_DebugInfo.drawCount =
			drawCount;

		g_DebugInfo.updateCount =
			updateCount;

		drawCount = 0;
		updateCount = 0;

		oldTime = currentTime;
	}
}