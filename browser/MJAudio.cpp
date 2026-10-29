
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
}

static inline std::string getKatipoResourcePath(const std::string& inPath, const tui::table& rootTable) //todo this is copied from MJCache
{
    tui::ref getResourcePathFunc = tui::toTable(rootTable->get("file"))->get("getResourcePath");
    if(getResourcePathFunc)
    {
        tui::ref pathResult = tui::toFunction(getResourcePathFunc)->call("getResourcePathFunc", tui::newString(inPath));
        if(pathResult)
        {
            std::string returnResult = pathResult->stringValue();
            return returnResult;
        }
    }
    return Katipo::getResourcePath(inPath);
}

void MJAudio::bindTui(const tui::table& rootTable)
{
    audioTable = tui::newTable(rootTable);
    rootTable->setTable("audio", audioTable);
    
    audioTable->onSet = [this](const tui::ref& table, const std::string& key, const tui::ref& value) {
        audioTableKeyChanged(key, value);
    };
    
    audioTable->setFunction("stop", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
#ifdef __APPLE__
        MJAudioApple::getInstance()->stop();
#else
        MJAudioSDLMixer::getInstance()->stop();
#endif
        return TUI_NIL;
    });
    
    
    audioTable->setFunction("playSongs", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
#ifdef __APPLE__
        MJAudioApple::getInstance()->play(nullptr);
#else
        MJAudioSDLMixer::getInstance()->playSongs(nullptr);
#endif
        return TUI_NIL;
    });
    
    
    
    audioTable->setFunction("playSound", [this, rootTable](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            double volume = 1.0;
            double pitch = 1.0;
            if(args->arrayObjects.size() >= 2 && args->arrayObjects[1]->isNumber())
            {
                volume = args->arrayObjects[1]->numberValue();
                if(args->arrayObjects.size() >= 3 && args->arrayObjects[2]->isNumber())
                {
                    pitch = args->arrayObjects[2]->numberValue();
                }
            }
            std::string path = getKatipoResourcePath(tui::toString(args->arrayObjects[0])->value, rootTable);
            MJAudioSDLMixer::getInstance()->playSound(path, volume, pitch);
        }
        else
        {
            TuiParseError(callingDebugInfo, "Incorrect argument type");
        }
        return TUI_NIL;
    });
    
    audioTable->setFunction("currentTrackTime", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
#ifdef __APPLE__
        return tui::newNumber(MJAudioApple::getInstance()->currentTrackTime());
#else
        return tui::newNumber(MJAudioSDLMixer::getInstance()->currentTrackTime());
#endif
        
        return TUI_NIL;
    });
    
    audioTable->setFunction("currentTrackDuration", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
#ifdef __APPLE__
        return tui::newNumber(MJAudioApple::getInstance()->currentTrackDuration());
#else
        return tui::newNumber(MJAudioSDLMixer::getInstance()->currentTrackDuration());
#endif
        
        return TUI_NIL;
    });
    
    
    audioTable->setFunction("currentTrackPath", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
#ifdef __APPLE__
        return tui::newString(MJAudioApple::getInstance()->currentTrackPath());
#else
        return tui::newString(MJAudioSDLMixer::getInstance()->currentTrackPath());
#endif
        
        return TUI_NIL;
    });
    
    audioTable->setFunction("next", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
#ifdef __APPLE__
        MJAudioApple::getInstance()->skipToNextTrack();
#else
        MJAudioSDLMixer::getInstance()->skipToNextTrack();
#endif
        
        return TUI_NIL;
    });
    
    audioTable->setFunction("seekToTime", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 1)
        {
            tui::ref timeNumberRef = args->arrayObjects[0];
            if(timeNumberRef->isNumber())
            {
#ifdef __APPLE__
                MJAudioApple::getInstance()->seekToTime(timeNumberRef->numberValue());
#else
                MJAudioSDLMixer::getInstance()->seekToTime(timeNumberRef->numberValue());
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
    
    audioTable->setFunction("setPlaybackRate", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 1)
        {
            tui::ref timeNumberRef = args->arrayObjects[0];
            if(timeNumberRef->isNumber())
            {
#ifdef __APPLE__
                MJAudioApple::getInstance()->setPlaybackRate(timeNumberRef->numberValue());
#else
                MJAudioSDLMixer::getInstance()->setPlaybackRate(timeNumberRef->numberValue());
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
    
    audioTable->setFunction("queueSongs", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 1)
        {
            tui::ref arrayRef = args->arrayObjects[0];
            if(arrayRef->type() == Tui_ref_type_TABLE)
            {
#ifdef __APPLE__
                MJAudioApple::getInstance()->play(tui::toTable(arrayRef));
#else
                MJAudioSDLMixer::getInstance()->playSongs(tui::toTable(arrayRef));
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

void MJAudio::audioTableKeyChanged(const std::string& key, const tui::ref& value)
{
    if(key == "playingSongChanged")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                playingSongChangedFunction = tui::toFunction(value);
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(playingSongChangedFunction)
                {
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
                playingSongPausedChangedFunction = tui::toFunction(value);
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(playingSongPausedChangedFunction)
                {
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
        playingSongChangedFunction->call("updateCurrentlyPlayingTrackInfo", tui::newString(titleString), tui::newString(artistString), tui::newNumber(duration));
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
