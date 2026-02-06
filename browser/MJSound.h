//todo this class is a stub
#ifndef MJSound_h
#define MJSound_h

#include <string>

class MJAudio;

struct MJChannelInfo {
	double volume;
	bool pausedDueToZeroVolume = false;
	bool pausedDueToAPI = false;
	//FMOD::DSP* lowPassEffect = nullptr;
};

class MJSound {

public: // members
    
protected: // members
    MJAudio* audio;
    //FMOD::Sound      *sound;
	std::string debugName;
    
    //std::map<uint32_t, FMOD::Channel*> channels;
	//std::map<uint32_t, MJChannelInfo> channelInfos;
    
    //uint32_t channelUniqueIDCount;
    
    bool looping;
	int priority = 128;

	//MJLuaRef* completionLuaFunction = nullptr;
    
public: // functions
    MJSound(MJAudio* audio);
    virtual ~MJSound();
    
    bool playing(uint32_t channelID = 0);
    void stop(uint32_t channelID = 0);
    
    bool paused(uint32_t channelID = 0);
    void setPaused(bool paused, uint32_t channelID = 0);
	void setPitch(double pitch, uint32_t channelID = 0);
	void setVolume(double volume, uint32_t channelID = 0);
	void setLowPassFilter(bool filter, double dryWetMix, uint32_t channelID = 0);
	void setPriority(int priority, uint32_t channelID = 0);
    
    void setLooping(bool looping_);
    
    void updateVolume(uint32_t channelID = 0);

    
    //virtual void channelFinishedPlaying(uint32_t uniqueID, FMOD::Channel *channel);

	//LuaRef getcompletionLuaFunction(lua_State* luaState) const GET_MJ_LUA_DEFINITION(completionLuaFunction)
	//void setcompletionLuaFunction(LuaRef ref) SET_MJ_LUA_DEFINITION(completionLuaFunction)
	//void removeCompletionLuaFunction() {if(completionLuaFunction) { delete completionLuaFunction; completionLuaFunction = nullptr;}}
    
protected: // functions
    
    //uint32_t playCommon(FMOD::Channel* channel_, double volume, double pitch);
    //virtual void cleanupChannel(uint32_t uniqueID);
    
    virtual float getCombinedVolume(uint32_t channelID = 0) = 0;

};

#endif /* MJSound_h */
