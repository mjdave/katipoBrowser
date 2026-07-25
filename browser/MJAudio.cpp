
#include "MJAudio.h"
#include "TuiScript.h"
#include "KatipoUtilities.h"

#ifdef __APPLE__
#include "MJAudioApple.h"
#endif

#include "MJAudioSDLMixer.h"


MJAudio::MJAudio()
{
}


MJAudio::~MJAudio()
{
    if(audioTable)
    {
        audioTable->release();
    }
}

static inline std::string getKatipoResourcePath(const std::string& inPath, TuiTable* rootTable) //todo this is copied from MJCache
{
    TuiRef* getResourcePathFunc = ((TuiTable*)rootTable->get("file"))->get("getResourcePath");
    if(getResourcePathFunc)
    {
        TuiString* inPathRef = new TuiString(inPath);
        TuiRef* pathResult = ((TuiFunction*)getResourcePathFunc)->call("getResourcePathFunc", inPathRef);
        inPathRef->release();
        if(pathResult)
        {
            std::string returnResult = pathResult->getStringValue();
            pathResult->release();
            return returnResult;
        }
    }
    return Katipo::getResourcePath(inPath);
}

void MJAudio::bindTui(TuiTable* rootTable)
{
    if(audioTable)
    {
        MJError("MJAudio::bindTui must only be called once");
    }
    audioTable = new TuiTable(rootTable);
    rootTable->setTable("audio", audioTable);
    
    audioTable->onSet = [this](TuiRef* table, const std::string& key, TuiRef* value) {
        audioTableKeyChanged(key, value);
    };
    
    audioTable->setFunction("stop", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        MJAudioApple::getInstance()->stop();
#else
        MJAudioSDLMixer::getInstance()->stop();
#endif
        return TUI_NIL;
    });
    
    
    audioTable->setFunction("playSongs", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        MJAudioApple::getInstance()->play(nullptr);
#else
        MJAudioSDLMixer::getInstance()->playSongs(nullptr);
#endif
        return TUI_NIL;
    });
    
    
    
    audioTable->setFunction("playSound", [this, rootTable](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            double volume = 1.0;
            double pitch = 1.0;
            if(args->arrayObjects.size() >= 2 && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
            {
                volume = ((TuiNumber*)args->arrayObjects[1])->value;
                if(args->arrayObjects.size() >= 3 && args->arrayObjects[2]->type() == Tui_ref_type_NUMBER)
                {
                    pitch = ((TuiNumber*)args->arrayObjects[2])->value;
                }
            }
            std::string path = getKatipoResourcePath(((TuiString*)args->arrayObjects[0])->value, rootTable);
            MJAudioSDLMixer::getInstance()->playSound(path, volume, pitch);
        }
        else
        {
            TuiParseError(callingDebugInfo, "Incorrect argument type");
        }
        return TUI_NIL;
    });
    
    audioTable->setFunction("currentTrackTime", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        return new TuiNumber(MJAudioApple::getInstance()->currentTrackTime());
#else
        return new TuiNumber(MJAudioSDLMixer::getInstance()->currentTrackTime());
#endif
        
        return TUI_NIL;
    });
    
    audioTable->setFunction("currentTrackDuration", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        return new TuiNumber(MJAudioApple::getInstance()->currentTrackDuration());
#else
        return new TuiNumber(MJAudioSDLMixer::getInstance()->currentTrackDuration());
#endif
        
        return TUI_NIL;
    });
    
    audioTable->setFunction("next", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        MJAudioApple::getInstance()->skipToNextTrack();
#else
        MJAudioSDLMixer::getInstance()->skipToNextTrack();
#endif
        
        return TUI_NIL;
    });
    
    audioTable->setFunction("seekToTime", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* timeNumberRef = args->arrayObjects[0];
            if(timeNumberRef->type() == Tui_ref_type_NUMBER)
            {
#ifdef __APPLE__
                MJAudioApple::getInstance()->seekToTime(((TuiNumber*)timeNumberRef)->value);
#else
                MJAudioSDLMixer::getInstance()->seekToTime(((TuiNumber*)timeNumberRef)->value);
#endif
            }
            else
            {
                TuiParseError(callingDebugInfo, "Incorrect argument type");
            }
        }
        else
        {
            TuiParseError(callingDebugInfo, "Missing args");
        }
        return TUI_NIL;
    });
    
    audioTable->setFunction("queueSongs", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* arrayRef = args->arrayObjects[0];
            if(arrayRef->type() == Tui_ref_type_TABLE)
            {
#ifdef __APPLE__
                MJAudioApple::getInstance()->play((TuiTable*)arrayRef);
#else
                MJAudioSDLMixer::getInstance()->playSongs((TuiTable*)arrayRef);
#endif
                MJLog("playing song");
            }
            else
            {
                TuiParseError(callingDebugInfo, "Incorrect argument type");
            }
        }
        else
        {
            TuiParseError(callingDebugInfo, "Missing args");
        }
        return TUI_NIL;
    });
}

void MJAudio::audioTableKeyChanged(const std::string& key, TuiRef* value)
{
    if(key == "playingSongChanged")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(playingSongChangedFunction)
                {
                    playingSongChangedFunction->release();
                }
                playingSongChangedFunction = (TuiFunction*)value;
                playingSongChangedFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(playingSongChangedFunction)
                {
                    playingSongChangedFunction->release();
                    playingSongChangedFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
    else if(key == "playingSongPausedChanged")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(playingSongPausedChangedFunction)
                {
                    playingSongPausedChangedFunction->release();
                }
                playingSongPausedChangedFunction = (TuiFunction*)value;
                playingSongPausedChangedFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(playingSongPausedChangedFunction)
                {
                    playingSongPausedChangedFunction->release();
                    playingSongPausedChangedFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
}

void MJAudio::updateCurrentlyPlayingInfo(const std::string& titleString, const std::string& artistString, double duration, void* imageBytes, int imageLength)
{
    if(playingSongChangedFunction)
    {
        TuiRef* titleRef = new TuiString(titleString);
        TuiRef* artistRef = new TuiString(artistString);
        TuiRef* durationRef = new TuiNumber(duration);
        
        playingSongChangedFunction->call("updateCurrentlyPlayingTrackInfo", titleRef, artistRef, durationRef);
        
        titleRef->release();
        artistRef->release();
        durationRef->release();
    }
#ifdef __APPLE__
    MJAudioApple::getInstance()->updateCurrentlyPlayingOSInfo(titleString, artistString, duration, 0, imageBytes, imageLength);
#endif
}

void MJAudio::updatePausedState(bool pauseSate)
{
    if(playingSongPausedChangedFunction)
    {
        playingSongPausedChangedFunction->call("updatePausedState", TUI_BOOL(pauseSate));
    }
}


void MJAudio::appLostFocus()
{
}

void MJAudio::appGainedFocus()
{
}
