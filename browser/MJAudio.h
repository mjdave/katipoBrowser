//Todo the audio system is very much a WIP
#ifndef MJAudio_h
#define MJAudio_h

#include <map>
#include <string>
#include <set>
#include "MathUtils.h"
#include "SDL.h"
#include "TuiScript.h"

class MJSound;
class MJSound2D;
class MJSound3D;
class MJSong;
class TuiTable;
class MIX_Mixer;

struct QueuedSong {
	MJSong* song;
	double volume;
};

class MJAudio {

public: // members
    
    dvec3 origin;
    dvec3 playerPos;
    dvec3 playerDirection;
    
    double overallVolume;
    double musicVolume;
    double soundVolume;
    
protected: // members
    SDL_AudioDeviceID audioDeviceId;
    MIX_Mixer* mixer;
    
    std::map<std::string, MJSound2D*> sounds2D;
    std::map<std::string, MJSound3D*> sounds3D;
    std::map<std::string, MJSong*> songs;
    
    std::set<MJSong*> pausedSongsDueToAppLosingFocus;
    std::set<MJSound2D*> pausedSounds2DDueToAppLosingFocus;
    std::set<MJSound3D*> pausedSounds3DDueToAppLosingFocus;

	QueuedSong queuedSong;
	bool hasQueuedSong = false;
    
    
    TuiFunction* playingSongChangedFunction = nullptr;
    TuiFunction* playingSongPausedChangedFunction = nullptr;

public: // functions
    
    
    static MJAudio* getInstance() {
        static MJAudio* instance = new MJAudio();
        return instance;
    }
    
    MJAudio();
    ~MJAudio();
    
    void emptyCache(bool fadeOut);
    void bindTui(TuiTable* rootTable);
    void audioTableKeyChanged(const std::string& key, TuiRef* value);
    void updateCurrentlyPlayingInfo(const std::string& titleString, const std::string& artistString, double duration, void* imageBytes, int imageLength);
    void updatePausedState(bool pauseSate);
    
    void appLostFocus();
    void appGainedFocus();
    
    void update();
    void updatePlayerPos(dvec3 playerPos_,
                         dvec3 playerDirection_,
                         dvec3 playerUp);
    void setOrigin(dvec3 origin);
    
    double getOverallVolume() const;
    void setOverallVolume(double overallVolume_);
    
    double getMusicVolume() const;
    void setMusicVolume(double musicVolume_);
    
    double getSoundVolume() const;
    void setSoundVolume(double soundVolume_);
    
    MJSound2D* loadSound2D(std::string name, bool loadInBackground);
    MJSound3D* loadSound3D(std::string name, bool loadInBackground);
    MJSong* loadSong(std::string name, bool loadInBackground);
    
    dvec3 worldPosToLocal(dvec3 worldPos);

	void deleteSound(MJSound* sound);

	void queueSong(MJSong* song, double volume);

	//LuaRef getQueuedOrPlayingSong();
	void fadeOutAnyCurrentSong();

    void songWillBeDestroyed(MJSong* song);
    void sound2DWillBeDestroyed(MJSound2D* sound);
    void sound3DWillBeDestroyed(MJSound3D* sound);

protected: // functions
    
    void updateSoundVolumes();
    void updateSongVolumes();

};

#endif /* MJAudio_h */
