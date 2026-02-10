
#include "MJAudio.h"
#include "TuiScript.h"
#include "KatipoUtilities.h"

#ifdef __APPLE__
#include "MJAudioApple.h"
#else
#include "MJAudioSDLMixer.h"
#endif


MJAudio::MJAudio()
{
}


MJAudio::~MJAudio()
{
}


void MJAudio::bindTui(TuiTable* rootTable)
{
    
    TuiTable* audioTable = new TuiTable(rootTable);
    rootTable->setTable("audio", audioTable);
    audioTable->release();
    
    audioTable->onSet = [this](TuiRef* table, const std::string& key, TuiRef* value) {
        audioTableKeyChanged(key, value);
    };
    
    audioTable->setFunction("stop", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        MJAudioApple::getInstance()->stop();
#else
        MJAudioSDLMixer::getInstance()->stop();
#endif
        return TUI_NIL;
    });
    
    
    audioTable->setFunction("play", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        MJAudioApple::getInstance()->play(nullptr);
#else
        MJAudioSDLMixer::getInstance()->play(nullptr);
#endif
        return TUI_NIL;
    });
    
    
    audioTable->setFunction("next", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        MJAudioApple::getInstance()->skipToNextTrack();
#else
        MJAudioSDLMixer::getInstance()->skipToNextTrack();
#endif
        
        return TUI_NIL;
    });
    
    audioTable->setFunction("queueSongs", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* arrayRef = args->arrayObjects[0];
            if(arrayRef->type() == Tui_ref_type_TABLE)
            {
#ifdef __APPLE__
                MJAudioApple::getInstance()->play((TuiTable*)arrayRef);
#else
                MJAudioSDLMixer::getInstance()->play((TuiTable*)arrayRef);
#endif
                MJLog("playing song");
            }
            else
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "Incorrect argument type");
            }
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "Missing args");
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
