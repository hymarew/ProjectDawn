#pragma once

struct DebugInfo
{
	float FPS = 0.0f;

	float timeScale = 1.0f;

	int updateCount = 0;

	int drawCount = 0;

	float frameTime = 0.0f;

	float accumulator = 0.0f;

	int maxUpdateReached = 0;
};

extern DebugInfo g_DebugInfo;

void InitDebugInfo();
void UpdateDebugInfo();
void AddUpdateCount();
void AddDrawCount();