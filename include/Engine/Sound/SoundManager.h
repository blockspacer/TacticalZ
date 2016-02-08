#ifndef SoundManager_h__
#define SoundManager_h__

#include <unordered_map>
#include <random>

#include "glm/common.hpp"
#include "glm/gtx/rotate_vector.hpp" // Calculate Up vector
#include "OpenAL/al.h"
#include "OpenAL/alc.h"

#include "imgui/imgui.h"

#include "Core/World.h"
#include "Core/EventBroker.h"
#include "../Engine/Core/ResourceManager.h"
#include "../Engine/Core/ConfigFile.h"
#include "Core/Transform.h" // Absolute transform
#include "Sound/Sound.h"
#include "../Engine/Sound/EPlayQueueOnEntity.h"
#include "Sound/EPlaySoundOnEntity.h"
#include "Sound/EPlaySoundOnPosition.h"
#include "Sound/EPlayBackgroundMusic.h"
#include "Sound/EPauseSound.h"
#include "Sound/EContinueSound.h"
#include "Sound/EStopSound.h"
#include "Sound/ESetBGMGain.h"
#include "Sound/ESetSFXGain.h"
#include "Core/EPause.h"
#include "Core/EComponentAttached.h"
#include "../Core/EPlayerSpawned.h"

typedef std::pair<ALuint, std::vector<ALuint>> QueuedBuffers;

enum class SoundType {
    SFX,
    BGM
};

struct Source
{
    Source() { }
    Sound* SoundResource = nullptr;
    ALuint ALsource;
    SoundType Type;
};

class SoundManager
{
public:
    SoundManager() { }
    SoundManager(World* world, EventBroker* eventBroker, bool editorMode);
    ~SoundManager();
    // Update emitters / listener
    void Update(double dt); 

private:
    // Help functions for working with OpenaAL
    void setListenerPos(glm::vec3 pos) { alListener3f(AL_POSITION, pos.x, pos.y, pos.z); };
    void setListenerVel(glm::vec3 vel) { alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z); };
    void setListenerOri(glm::vec3 ori);
    glm::vec3 listnerPos() { glm::vec3 pos; alGetListener3f(AL_POSITION, &pos.x, &pos.y, &pos.z); return pos; };
    glm::vec3 listenerVel() { glm::vec3 vel; alGetListener3f(AL_VELOCITY, &vel.x, &vel.y, &vel.z); return vel; };
    glm::vec3 listenerOri() { glm::vec3 ori; alGetListener3f(AL_ORIENTATION, &ori.x, &ori.y, &ori.z); return ori; };
    void setSourcePos(ALuint source, glm::vec3 pos) { ALfloat spos[3] = { pos.x, pos.y, pos.z };  alSourcefv(source, AL_POSITION, spos); };
    void setSourceVel(ALuint source, glm::vec3 vel) { ALfloat svel[3] = { vel.x, vel.y, vel.z };  alSourcefv(source, AL_VELOCITY, svel); };

    // Logic 
    void initOpenAL();
    void updateEmitters(double dt);
    void deleteInactiveEmitters();
    void stopEmitters();
    void updateListener(double dt);
    ALenum getSourceState(ALuint source);
    void setGain(Source* source, float gain);
    void setSoundProperties(Source* source, ComponentWrapper* soundComponent);

    // Specific logic
    void playSound(Source* source);
    // Need to be the same format (sample rate etc)
    void playQueue(QueuedBuffers qb);
    void stopSound(Source* source);
    Source* createSource(std::string filePath);
    std::unordered_map<EntityID, Source*> m_Sources;

    // Logic
    World* m_World = nullptr;
    EventBroker* m_EventBroker = nullptr;

    // OpenAL system variables
    ALCdevice* m_ALCdevice = nullptr;
    ALCcontext* m_ALCcontext = nullptr;

    float m_BGMVolumeChannel = 1.0f;
    float m_SFXVolumeChannel = 1.0f;
    bool m_EditorEnabled = false;
    EntityWrapper m_LocalPlayer = EntityWrapper();

    // Events
    EventRelay<SoundManager, Events::PlaySoundOnEntity> m_EPlaySoundOnEntity;
    bool OnPlaySoundOnEntity(const Events::PlaySoundOnEntity &e);
    EventRelay<SoundManager, Events::PlaySoundOnPosition> m_EPlaySoundOnPosition;
    bool OnPlaySoundOnPosition(const Events::PlaySoundOnPosition &e);
    EventRelay<SoundManager, Events::PlayBackgroundMusic> m_EPlayBackgroundMusic;
    bool OnPlayBackgroundMusic(const Events::PlayBackgroundMusic &e);
    EventRelay<SoundManager, Events::PauseSound> m_EPauseSound;
    bool OnPauseSound(const Events::PauseSound &e);
    EventRelay<SoundManager, Events::StopSound> m_EStopSound;
    bool OnStopSound(const Events::StopSound &e);
    EventRelay<SoundManager, Events::ContinueSound> m_EContinueSound;
    bool OnContinueSound(const Events::ContinueSound &e);
    EventRelay<SoundManager, Events::SetBGMGain> m_ESetBGMGain; 
    bool OnSetBGMGain(const Events::SetBGMGain &e);
    EventRelay<SoundManager, Events::SetSFXGain> m_ESetSFXGain; 
    bool OnSetSFXGain(const Events::SetSFXGain &e);
    EventRelay<SoundManager, Events::ComponentAttached> m_EComponentAttached;
    bool OnComponentAttached(const Events::ComponentAttached &e);
    EventRelay<SoundManager, Events::Pause> m_EPause;
    bool OnPause(const Events::Pause &e);
    EventRelay<SoundManager, Events::Resume> m_EResume;
    bool OnResume(const Events::Resume &e);
    EventRelay<SoundManager, Events::PlayerSpawned> m_EPlayerSpawned;
    bool OnPlayerSpawned(const Events::PlayerSpawned &e);
    EventRelay<SoundManager, Events::PlayQueueOnEntity> m_EPlayQueueOnEntity;
    bool OnPlayQueueOnEntity(const Events::PlayQueueOnEntity &e);


};

#endif