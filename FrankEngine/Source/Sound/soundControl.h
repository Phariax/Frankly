////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Sound Engine
	Copyright 2013 Frank Force - http://www.frankforce.com

	- low level sound code for Frank lib
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef SOUND_CONTROL_H
#define SOUND_CONTROL_H

#include "dsound.h"
#include "../sound/musicControl.h"
#include "../objects/gameObject.h"
#include "../DXUT/optional/SDKsound.h"

enum SoundControl_ID;
const SoundControl_ID SoundControl_Invalid = SoundControl_ID(0);
const SoundControl_ID SoundControl_Start = SoundControl_ID(1);

typedef unsigned SoundObjectHandle;

class SoundObject
{
	typedef stdext::hash_map<SoundObjectHandle, SoundObject*> SoundObjectHashTable;
	typedef pair<SoundObjectHandle, class SoundObject*> SoundObjectHashPair;

public:

	SoundObject();
	void Stop();
	void Update(GameObject& object, float frequencyScale = 1, float volumePercent = 1);
	bool IsPlaying() const { return bufferWrapper != NULL; }

	CSound::BufferWrapper* bufferWrapper;
	float distanceMin;
	float distanceMax;

	SoundObjectHandle GetHandle() const { return handle; }

	static void GlobalUpdate();

	static SoundObject* GetFromHandle(SoundObjectHandle handle);
	static void ResetHandles();

private:
	
	SoundObjectHandle handle;							// the unique handle of this object

	static SoundObjectHashTable soundObjectMap;			// hash map of all sound sounds
	static SoundObjectHandle nextUniqueHandleValue;		// used only internaly to give out unique handles

};

class SoundControl : private Uncopyable
{
public:

	SoundControl();
	~SoundControl();

	void InitDeviceObjects();
	void DestroyDeviceObjects();

	bool LoadSound(WCHAR* soundName, SoundControl_ID si, DWORD soundCreationFlags = 0, DWORD bufferCount = maxBufferCount, bool applyTimeScale = true);
	void ReleaseSounds();

	void Play(SoundControl_ID si, float volumePercent = 1.0f, float frequencyScale = 1.0f, float frequencyRandomness = defaultFrequencyRandomness, SoundObjectHandle* soundHandle = NULL, bool loop = false);

	void Play
	(
		SoundControl_ID si,
		const GameObject& object, 
		float volumePercent = 1,
		float frequencyScale = 1,
		float frequencyRandomness = defaultFrequencyRandomness,
		SoundObjectHandle* soundHandle = NULL,
		float distanceMin = defaultDistanceMin, 
		float distanceMax = defaultDistanceMax,
		bool loop = false
	);

	void Play
	(
		SoundControl_ID si,
		const Vector2& position, 
		float volumePercent = 1,
		float frequencyScale = 1,
		float frequencyRandomness = defaultFrequencyRandomness,
		SoundObjectHandle* soundHandle = NULL,
		const Vector2& velocity = Vector2::Zero(), 
		bool listenerRelative = false,
		float distanceMin = defaultDistanceMin, 
		float distanceMax = defaultDistanceMax,
		bool loop = false
	);
	
	void StopSounds();

	void Update();

	void UpdateTimeScale(float scale);

	MusicControl& GetMusicPlayer() { return musicPlayer; }

	void SetDopplerFactor(float d) { dopplerFactor = d; }
	
	void SetVolumeScale( float _volumeScale )		{ volumeScale = _volumeScale; }
	float GetVolumeScale() const					{ return volumeScale; }

	static float masterVolume;
	static float rollOffFactor;
	static float dopplerFactor;
	static float dopplerDistanceFactor;
	static bool enableSoundTimeScale;	// should we update sounds with the time scale
	static bool cameraIsListener;		// is camera or player listener

	static const int maxSoundCount = 256;
	static const int maxBufferCount = 8;
	static float defaultDistanceMin;
	static float defaultDistanceMax;
	static float defaultFrequencyRandomness;

private:

	MusicControl musicPlayer;
	bool hasPlayedSound;			// hack to prevent listener update before sound is played
	float volumeScale;				// control for scaling music volume

	class CSoundManager* soundManager;

	struct SoundWrapper
	{
		WCHAR name[FILENAME_STRING_LENGTH];
		class CSound* sound;
		bool applyTimeScale;
	};

	SoundWrapper sounds[maxSoundCount];
};

#endif // SOUND_CONTROL_H