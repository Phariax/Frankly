////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Sound Engine
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../sound/soundControl.h"

static bool soundControlEnable = true;
ConsoleCommand(soundControlEnable, soundEnable);

float SoundControl::masterVolume = 0.85f;
ConsoleCommand(SoundControl::masterVolume, soundVolume);

float SoundControl::defaultDistanceMin = 5;
ConsoleCommand(SoundControl::defaultDistanceMin, soundDefaultDistanceMin);

float SoundControl::defaultDistanceMax = DS3D_DEFAULTMAXDISTANCE;
ConsoleCommand(SoundControl::defaultDistanceMax, soundDefaultDistanceMax);

float SoundControl::rollOffFactor = 2.0f;
ConsoleCommand(SoundControl::rollOffFactor, soundRollOffFactor);

float SoundControl::dopplerFactor = 1.0f;
ConsoleCommand(SoundControl::dopplerFactor, soundDopplerFactor);

float SoundControl::dopplerDistanceFactor = 1.0f;
ConsoleCommand(SoundControl::dopplerDistanceFactor, soundDopplerDistanceFactor);

bool SoundControl::enableSoundTimeScale = true;
ConsoleCommand(SoundControl::enableSoundTimeScale, enableSoundTimeScale);

float SoundControl::defaultFrequencyRandomness	= 0.05f;
ConsoleCommand(SoundControl::defaultFrequencyRandomness, soundDefaultFrequencyRandomness);

bool SoundControl::cameraIsListener = true;
ConsoleCommand(SoundControl::cameraIsListener, cameraIsListener);

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Sound Member Functions
*/
////////////////////////////////////////////////////////////////////////////////////////

SoundControl::SoundControl() :
	hasPlayedSound(false),
	soundManager(NULL)
{
	ASSERT(!g_sound);

	soundManager = new CSoundManager;

	if (!soundManager)
		return;

    // We need to set up DirectSound after we have a window.
    soundManager->Initialize( DXUTGetHWND(), DSSCL_PRIORITY );

	// clear the sound list
	for (int i = 0; i < maxSoundCount; i++)
	{
		sounds[i].sound = NULL;
		sounds[i].name[0] = L'\0';
		sounds[i].applyTimeScale = true;
	}

	musicPlayer.SetDirectSound(soundManager->GetDirectSound());
}

SoundControl::~SoundControl()
{
	if (!soundManager)
		return;

	musicPlayer.ShutDown();

	ReleaseSounds();

	SAFE_DELETE(soundManager);
}

void SoundControl::ReleaseSounds()
{
	if (!soundManager)
		return;

	// clear the sound list
	for (int i = 0; i < maxSoundCount; i++)
	{
		SAFE_DELETE(sounds[i].sound);
		sounds[i].name[0] = L'\0';
	}

	SoundObject::ResetHandles();
}

bool SoundControl::LoadSound(WCHAR* soundName, SoundControl_ID si, DWORD soundCreationFlags, DWORD bufferCount, bool applyTimeScale)
{
	if (!soundManager)
		return false;

	ASSERT(si < maxSoundCount);

	// check if the sound already exists
	if (sounds[si].sound)
		return true;

	// if we have no flags force it to be a 3d sound
	if (!soundCreationFlags)
		soundCreationFlags = DSBCAPS_CTRL3D|DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLVOLUME;

	/*
	// search through all the sub folders to find the file
	WCHAR sPath[MAX_PATH];
	HRESULT hr = DXUTFindDXSDKMediaFileCch( sPath, MAX_PATH, soundNameWav );
	if (FAILED(hr))
		StringCchCopy( sPath, MAX_PATH, soundName );
	*/

	// hack: check sound folder
	// todo: search subfolders, or way to add list of sub folders to search
	WCHAR soundName2[FILENAME_STRING_LENGTH];
	wcsncpy_s(soundName2, FILENAME_STRING_LENGTH, L"data/sound/", _TRUNCATE );
	wcscat_s(soundName2, FILENAME_STRING_LENGTH, soundName);

	HRESULT hr = soundManager->Create( &sounds[si].sound, soundName2, soundCreationFlags, GUID_NULL, bufferCount );
	if (FAILED(hr))
	{
		hr = soundManager->Create( &sounds[si].sound, soundName, soundCreationFlags, GUID_NULL, bufferCount );

		if (FAILED(hr))
		{
			g_debugMessageSystem.AddError( L"Sound \"%s\" not found.\n", soundName );
			sounds[si].sound = NULL;
			return false;
		}
	}

	sounds[si].applyTimeScale = applyTimeScale;
	wcsncpy_s(sounds[si].name, FILENAME_STRING_LENGTH, soundName, FILENAME_STRING_LENGTH);
	return true;
}

void SoundControl::Play(SoundControl_ID si, float volumePercent, float frequencyScale, float frequencyRandomness, SoundObjectHandle* soundHandle, bool loop)
{
	if (!soundControlEnable)
		return;

	if (!soundManager)
		return;

	ASSERT(volumePercent >= 0 && volumePercent <= 1);

	CSound* sound = sounds[si].sound;
	if (!sound)
		return;
	
	// sound must not be created for 3d control cause it wont play
	ASSERT(!sound->Is3D());

	// add a bit of randomness to the frequency
	frequencyScale *= RAND_BETWEEN(1.0f - frequencyRandomness, 1.0f + frequencyRandomness);

	const float scale = enableSoundTimeScale && sounds[si].applyTimeScale? g_gameControlBase->GetTimeScale() : 1.0f;
	if (soundHandle)
	{
		SoundObject* soundObject = new SoundObject;
		sound->Play(0, loop? DSBPLAY_LOOPING : 0, volumePercent*masterVolume, frequencyScale, 0, &soundObject->bufferWrapper);
		sound->UpdateTimeScale(scale);
		*soundHandle = soundObject->GetHandle();
	}
	else
	{
		sound->Play(0, loop? DSBPLAY_LOOPING : 0, volumePercent*masterVolume, frequencyScale);
		if (scale != 1)
			sound->UpdateTimeScale(scale);
	}

	if (enableSoundTimeScale && sounds[si].applyTimeScale)
		sound->UpdateTimeScale(g_gameControlBase->GetTimeScale());

	hasPlayedSound = true;
}

void SoundControl::Play(SoundControl_ID si, const GameObject& object, float volumePercent, float frequencyScale, float frequencyRandomness, SoundObjectHandle* soundHandle, float distanceMin, float distanceMax, bool loop)
{
	const GameObject* physicalObject = object.GetAttachedPhysics();
	// special case if the object is the player/listener
	const bool listenerRelative = (physicalObject? *physicalObject == *g_gameControlBase->GetPlayer() : false);
	const Vector2 position = object.GetPosWorld();
	const Vector2 velocity = physicalObject? physicalObject->GetVelocity() : Vector2::Zero();

	Play(si, position, volumePercent, frequencyScale, frequencyRandomness, soundHandle, velocity, listenerRelative, distanceMin, distanceMax, loop);
}

void SoundControl::Play(SoundControl_ID si, const Vector2& position, float volumePercent, float frequencyScale, float frequencyRandomness, SoundObjectHandle* soundHandle, const Vector2& velocity, bool listenerRelative, float distanceMin, float distanceMax, bool loop)
{
	if (!soundControlEnable)
		return;

	if (!soundManager)
		return;

	CSound* sound = sounds[si].sound;
	if (!sound)
		return;

	ASSERT(volumePercent >= 0.0f && volumePercent <= 1.0f);

	const float distance = (position - g_gameControlBase->GetPlayer()->GetPosWorld()).Magnitude();
	if (!soundHandle && distance > distanceMax)
		return;	// sound is out of range

	// special case if the object is the player/listener
	const D3DVECTOR coneDirection = {0, 0, 1};
	const DWORD listenerMode = listenerRelative? DS3DMODE_DISABLE : DS3DMODE_NORMAL;

	volumePercent *= masterVolume;

	DS3DBUFFER sound3DInfo =
	{
		sizeof(DS3DBUFFER),				// dwSize
		position.GetD3DXVECTOR3(),		// position
		velocity.GetD3DXVECTOR3(),		// velocity
		DS3D_MINCONEANGLE,				// insideConeAngle
		DS3D_MAXCONEANGLE,				// outsideConeAngle
		coneDirection,					// coneOrientation
		DS3D_DEFAULTCONEOUTSIDEVOLUME,	// coneOutsideVolume
		distanceMin,					// minDistance
		distanceMax,					// maxDistance
		listenerMode					// dwMode
	};

	// add a bit of randomness to the frequency
	frequencyScale *= RAND_BETWEEN(1.0f - frequencyRandomness, 1.0f + frequencyRandomness);

	const float scale = enableSoundTimeScale && sounds[si].applyTimeScale? g_gameControlBase->GetTimeScale() : 1.0f;
	hasPlayedSound = true;

	if (soundHandle)
	{
		SoundObject* soundObject = new SoundObject;
		sound->Play3D(&sound3DInfo, 0, loop? DSBPLAY_LOOPING : 0, volumePercent, frequencyScale, &soundObject->bufferWrapper);
		sound->UpdateTimeScale(scale);
		soundObject->distanceMin = distanceMin;
		soundObject->distanceMax = distanceMax;
		*soundHandle = soundObject->GetHandle();
	}
	else
	{
		sound->Play3D(&sound3DInfo, 0, 0, volumePercent, frequencyScale);
		if (scale != 1)
			sound->UpdateTimeScale(scale);
	}
}

void SoundControl::Update()
{
	musicPlayer.Update();

	if (!soundManager)
		return;

	if (!g_gameControlBase->GetPlayer() || !g_cameraBase)
		return;

	// hack: only update listerner after first sound is played
	// this is because it was causeing the game to go super slow
	if (!hasPlayedSound)
		return;

	IDirectSound3DListener* sound3DListenerInterface;
	soundManager->Get3DListenerInterface( &sound3DListenerInterface );

	//const D3DXVECTOR3 position = g_gameControlBase->GetPlayer()->GetInterpolatedXForm().position.GetD3DXVECTOR3();
	const XForm2 cameraXF = g_cameraBase->GetInterpolatedXForm();
	D3DXVECTOR3 position;
	if (cameraIsListener || !g_gameControlBase->GetPlayer())
		position = cameraXF.position.GetD3DXVECTOR3();
	else
		position = g_gameControlBase->GetPlayer()->GetPosWorld().GetD3DXVECTOR3();

	const D3DXVECTOR3 velocity = g_gameControlBase->GetPlayer()->GetVelocity().GetD3DXVECTOR3();
	const D3DXVECTOR3 orientationFront(0, 0, 1);
	D3DXVECTOR3 orientationTop = cameraXF.GetUp().GetD3DXVECTOR3();

	DS3DLISTENER sound3DListener =
	{
		sizeof(DS3DLISTENER),	// dwSize
		position,				// position
		velocity,				// velocity
		orientationFront,		// orientFront
		orientationTop,			// orientTop
		dopplerDistanceFactor,	// distanceFactor
		rollOffFactor,			// rolloffFactor
		dopplerFactor			// dopplerFactor
	};
	
	sound3DListenerInterface->SetAllParameters( &sound3DListener, DS3D_IMMEDIATE );

	SoundObject::GlobalUpdate();
}

void SoundControl::UpdateTimeScale(float scale)
{
	if (!soundManager)
		return;

	// hack: only update listerner after first sound is played
	// this is because it was causeing the game to go super slow
	if (!hasPlayedSound)
		return;

	if (!enableSoundTimeScale)
		scale = 1.0f;

	if (g_gameControlBase->IsPaused())
		scale = 0.0f;	// set to min frequency, to "pause" the sound

	// update time scale
	for (int i = 0; i < maxSoundCount; i++)
	{
		if (!sounds[i].applyTimeScale)
			continue;

		if (sounds[i].sound)
			sounds[i].sound->UpdateTimeScale(scale);
	}
}

void SoundControl::StopSounds()
{
	if (!soundManager)
		return;

	// hack: only update listerner after first sound is played
	// this is because it was causeing the game to go super slow
	if (!hasPlayedSound)
		return;

	// update time scale
	for (int i = 0; i < maxSoundCount; i++)
	{
		if (sounds[i].sound)
		{
			sounds[i].sound->Stop();
			sounds[i].sound->Reset();
		}
	}
	
	SoundObject::ResetHandles();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SoundObjectHandle SoundObject::nextUniqueHandleValue = 1;	// starting unique handle
SoundObject::SoundObjectHashTable SoundObject::soundObjectMap;	// the global group containing all game objects

SoundObject::SoundObject() :
	bufferWrapper(NULL),
	handle(++nextUniqueHandleValue)
{
	soundObjectMap.insert(SoundObjectHashPair(handle, this));
}
	
void SoundObject::GlobalUpdate()
{ 
	for (SoundObjectHashTable::iterator it = soundObjectMap.begin(); it != soundObjectMap.end(); )
	{
		// remove sounds that aren't playing
		SoundObject& sound = *(*it).second;

		if (!sound.IsPlaying())
		{
			it = soundObjectMap.erase(it);
			delete &sound;
			continue;
		}

		++it;
	}
}

SoundObject* SoundObject::GetFromHandle(SoundObjectHandle handle)
{ 
	// look for matching handle
	SoundObjectHashTable::iterator it = soundObjectMap.find(handle);
	return (it != soundObjectMap.end())? ((*it).second) : NULL;
}

void SoundObject::Stop()
{
	if (!handle || !bufferWrapper)
		return;

	if (bufferWrapper->buffer)
	{
		bufferWrapper->buffer->Stop();
        bufferWrapper->buffer->SetCurrentPosition( 0 );
	}

	soundObjectMap.erase(handle);
	handle = 0;
	delete this;
}

void SoundObject::Update(GameObject& object, float frequencyScale, float volumePercent)
{
	if (!handle || !bufferWrapper || !bufferWrapper->buffer)
		return;

	if (!soundControlEnable)
	{
		if (bufferWrapper->buffer)
			bufferWrapper->buffer->Stop();
		return;
	}

	// special case if the object is the player/listener
	const GameObject* physicalObject = object.GetAttachedPhysics();
	const bool listenerRelative = (physicalObject? *physicalObject == *g_gameControlBase->GetPlayer() : false);
	const D3DXVECTOR3 position = object.GetPosWorld().GetD3DXVECTOR3();
	const D3DXVECTOR3 velocity = physicalObject? physicalObject->GetVelocity().GetD3DXVECTOR3() : Vector3::Zero();
	const D3DVECTOR coneDirection = {0, 0, 1};
	const DWORD listenerMode = listenerRelative? DS3DMODE_DISABLE : DS3DMODE_NORMAL;

	DS3DBUFFER sound3DInfo =
	{
		sizeof(DS3DBUFFER),				// dwSize
		position,						// position
		velocity,						// velocity
		DS3D_MINCONEANGLE,				// insideConeAngle
		DS3D_MAXCONEANGLE,				// outsideConeAngle
		coneDirection,					// coneOrientation
		DS3D_DEFAULTCONEOUTSIDEVOLUME,	// coneOutsideVolume
		distanceMin,					// minDistance
		distanceMax,					// maxDistance
		listenerMode					// dwMode
	};

	// QI for the 3D buffer
	LPDIRECTSOUND3DBUFFER pDS3DBuffer;
    HRESULT hr = bufferWrapper->buffer->QueryInterface( IID_IDirectSound3DBuffer, (VOID**) &pDS3DBuffer );
	if (SUCCEEDED(hr))
	{
		pDS3DBuffer->SetAllParameters( &sound3DInfo, DS3D_IMMEDIATE );
		pDS3DBuffer->Release();
	}

	bufferWrapper->frequencyScale = frequencyScale;
	
	LONG lVolume = (LONG)(DSBVOLUME_MIN + SoundControl::masterVolume*volumePercent * (DSBVOLUME_MAX - DSBVOLUME_MIN - 1));
	if (lVolume == 0)
		lVolume = 1;

	bufferWrapper->buffer->SetVolume( lVolume );
}
	
void SoundObject::ResetHandles()
{
	for (SoundObjectHashTable::iterator it = soundObjectMap.begin(); it != soundObjectMap.end(); ++it)
		delete (*it).second;
	soundObjectMap.clear();
}