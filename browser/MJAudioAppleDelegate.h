
#ifndef MJaudioAppleDelegate_h
#define MJaudioAppleDelegate_h

#import <AVFoundation/AVFoundation.h>
#import <MediaPlayer/MediaPlayer.h>
#include <vector>
#include "TuiScript.h"

@interface MJAudioAppleDelegate : NSObject
{
    AVQueuePlayer* player;
    bool hasUrls;
    
    id timeObserverToken;
    
    //std::map<AVPlayerItem*, int> queueIndexesByPlayerItems;
    //std::map<int, std::string> urlsByQueueIndex;
    std::map<AVPlayerItem*, AVPlayerItem*> nextPlayerItemByPlayerItem;
}

- (id)init;

- (void)updateInfo:(AVPlayerItem*)playerItem;

- (void)play:(TuiTable*)urls;
- (void)stop;
- (void)skipToNextTrack;

- (bool)paused;

- (MPRemoteCommandHandlerStatus )remoteCommandEvent:(MPRemoteCommandEvent *)event;

@end

#endif /* MJAudio_h */
