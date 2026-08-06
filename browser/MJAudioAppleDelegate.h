
#ifndef MJaudioAppleDelegate_h
#define MJaudioAppleDelegate_h

#import <AVFoundation/AVFoundation.h>
#import <MediaPlayer/MediaPlayer.h>
#include <vector>
#include "TuiScript.h"

@interface MJAudioAppleDelegate : NSObject
{
    AVQueuePlayer* player;
    double currentTrackDuration;
    bool hasUrls;
    
    id timeObserverToken;
    
    //std::map<AVPlayerItem*, int> queueIndexesByPlayerItems;
    //std::map<int, std::string> urlsByQueueIndex;
    std::map<AVPlayerItem*, AVPlayerItem*> nextPlayerItemByPlayerItem;
    std::map<AVPlayerItem*, std::string> filePathsByPlayerItems;
}

- (id)init;

- (void)updateInfo:(AVPlayerItem*)playerItem;

- (void)play:(TuiTable*)urls;
- (void)stop;
- (void)skipToNextTrack;

- (bool)paused;
- (double)currentTrackTime;
- (double)currentTrackDuration;
- (std::string)currentTrackPath;

- (void)seekToTime:(double)seekTime;
- (void)setPlaybackRate:(double)rate;

- (MPRemoteCommandHandlerStatus )remoteCommandEvent:(MPRemoteCommandEvent *)event;

@end

#endif /* MJAudio_h */
