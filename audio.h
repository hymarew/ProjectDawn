#pragma once

#include <xaudio2.h>
#include "component.h"


class Audio : public Component
{
private:
	static IXAudio2*				m_Xaudio;
	static IXAudio2MasteringVoice*	m_MasteringVoice;

	IXAudio2SourceVoice*	m_SourceVoice{};
	BYTE*					m_SoundData{};

	int						m_Length{};
	int						m_PlayLength{};


public:
	static void InitMaster();
	static void UninitMaster();

	using Component::Component;

	void Uninit();

	void Load(const wchar_t *FileName);
	void Play(bool Loop = false);
	void Stop();

	// 音量を設定する（0.0 = 無音, 1.0 = 原音量）。再生中でも即時反映される
	void SetVolume(float volume);


};

