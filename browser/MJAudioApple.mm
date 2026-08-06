
#include "MJAudioApple.h"
#import <AVFoundation/AVFoundation.h>
#import "MJAudioAppleDelegate.h"
#include "TuiScript.h"
#import <MediaPlayer/MediaPlayer.h>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

static NSMutableDictionary *currentlyPlayingTrackInfo = nullptr;

static MJAudioAppleDelegate* audioAppleDelegate = [[MJAudioAppleDelegate alloc] init];


static inline NSArray * RemoteCommandCenterCommandsToHandle(void)
{
    MPRemoteCommandCenter *cc = [MPRemoteCommandCenter sharedCommandCenter];
    NSMutableArray *commands = [NSMutableArray arrayWithObjects:
                                cc.pauseCommand,
                                cc.playCommand,
                                cc.stopCommand,
                                cc.togglePlayPauseCommand,
                                cc.nextTrackCommand,
                                cc.previousTrackCommand,
                                //cc.changePlaybackRateCommand,
                                nil];
    
        [commands addObject:cc.changePlaybackPositionCommand];
        [commands addObject:cc.changeShuffleModeCommand];
        //[commands addObject:cc.changeRepeatModeCommand];
    
    return [commands copy];
}


MJAudioApple::MJAudioApple()
{
    
}


MJAudioApple::~MJAudioApple()
{
    /*if(finishedCallbackFunction)
    {
        finishedCallbackFunction->release();
    }*/
}

/*void MJAudioApple::audioPlayerDidFinishPlaying(bool success)
{
    MJLog("got track finished callback")
    if(finishedCallbackFunction)
    {
        finishedCallbackFunction->call("audioPlayerDidFinishPlaying", TUI_BOOL(success));
        finishedCallbackFunction = nullptr;
    }
}*/

void MJAudioApple::play(TuiTable* urls)
{
    [audioAppleDelegate play:urls];
    MPRemoteCommandCenter *commandCenter = [MPRemoteCommandCenter sharedCommandCenter];
    
    /*
     
                                 cc.pauseCommand,
                                 cc.playCommand,
                                 cc.stopCommand,
                                 cc.togglePlayPauseCommand,
                                 cc.nextTrackCommand,
                                 cc.previousTrackCommand,
                                 //cc.changePlaybackRateCommand,
                                 nil];
     
         [commands addObject:cc.changePlaybackPositionCommand];
         [commands addObject:cc.changeShuffleModeCommand];
         //[commands addObject:cc.changeRepeatModeCommand];
     */
    
    commandCenter.pauseCommand.enabled = true;
    commandCenter.playCommand.enabled = true;
    commandCenter.stopCommand.enabled = true;
    commandCenter.togglePlayPauseCommand.enabled = true;

    commandCenter.nextTrackCommand.enabled = true;
    commandCenter.previousTrackCommand.enabled = true;
    commandCenter.togglePlayPauseCommand.enabled = true;
    commandCenter.changePlaybackPositionCommand.enabled = true;

    //commandCenter.changeRepeatModeCommand.enabled = YES;
    commandCenter.changeShuffleModeCommand.enabled = YES;

    for (MPRemoteCommand *command in RemoteCommandCenterCommandsToHandle()) {
        [command addTarget:audioAppleDelegate action:@selector(remoteCommandEvent:)];
    }

}

void MJAudioApple::stop()
{
    [audioAppleDelegate stop];
}


void MJAudioApple::skipToNextTrack()
{
    [audioAppleDelegate skipToNextTrack];
}

double MJAudioApple::currentTrackTime()
{
    return [audioAppleDelegate currentTrackTime];
}

double MJAudioApple::currentTrackDuration()
{
    return [audioAppleDelegate currentTrackDuration];
}


std::string MJAudioApple::currentTrackPath()
{
    return [audioAppleDelegate currentTrackPath];
}

void MJAudioApple::seekToTime(double seconds)
{
    [audioAppleDelegate seekToTime:seconds];
}


void MJAudioApple::setPlaybackRate(double rate)
{
    [audioAppleDelegate setPlaybackRate:rate];
}

void MJAudioApple::updatePausedState()
{
    if(currentlyPlayingTrackInfo)
    {
        currentlyPlayingTrackInfo[MPNowPlayingInfoPropertyPlaybackRate] = [NSNumber numberWithDouble:([audioAppleDelegate paused] ? 0.0 : 1.0)];
        [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = currentlyPlayingTrackInfo;
    }
}

void MJAudioApple::updateCurrentlyPlayingOSInfo(const std::string& titleString, const std::string& artistString, double trackDuration, double elapsedPlaybackTime, void* imageBytes, int imageLength)
{
    if(!currentlyPlayingTrackInfo)
    {
        currentlyPlayingTrackInfo = [[NSMutableDictionary alloc] init];
    }
    
    currentlyPlayingTrackInfo[MPMediaItemPropertyPlaybackDuration] = [NSNumber numberWithDouble:trackDuration];
    currentlyPlayingTrackInfo[MPNowPlayingInfoPropertyElapsedPlaybackTime] = [NSNumber numberWithDouble:elapsedPlaybackTime];

    currentlyPlayingTrackInfo[MPMediaItemPropertyTitle] = [NSString stringWithUTF8String:titleString.c_str()];
    currentlyPlayingTrackInfo[MPMediaItemPropertyArtist] = [NSString stringWithUTF8String:artistString.c_str()];//self.artist;
    //currentlyPlayingTrackInfo[MPMediaItemPropertyAlbumTitle] = @"Dave's Fantatstic Album";//self.albumName;
    currentlyPlayingTrackInfo[MPNowPlayingInfoPropertyPlaybackRate] = [NSNumber numberWithDouble:([audioAppleDelegate paused] ? 0.0 : 1.0)];
    
#if TARGET_OS_IPHONE
    UIImage* image = [[UIImage alloc] initWithData:[NSData dataWithBytes:imageBytes length:imageLength]];
    currentlyPlayingTrackInfo[MPMediaItemPropertyArtwork] = [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size requestHandler:^UIImage * _Nonnull(CGSize size) {
        return image;
    }];
#else
    NSImage* image = [[NSImage alloc] initWithData:[NSData dataWithBytes:imageBytes length:imageLength]];
    currentlyPlayingTrackInfo[MPMediaItemPropertyArtwork] = [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size requestHandler:^NSImage * _Nonnull(CGSize size) {
        return image;
    }];
                                                             //initWithImage:[[NSImage alloc] initWithData:[NSData dataWithBytes:imageBytes length:imageLength]]];
#endif

            //currentlyPlayingTrackInfo[MPMediaItemPropertyAlbumTrackNumber] = [NSNumber numberWithInt:trackNumber];

   /* #if TARGET_OS_IOS || TARGET_OS_VISION
        if (self.artworkImage) {
            MPMediaItemArtwork *mpartwork;
            if (@available(iOS 10.0 VISIONOS_AVAILABLE, *)) {
                mpartwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:self.artworkImage.size
                                                            requestHandler:^UIImage * _Nonnull(CGSize size) {
                    return self.artworkImage;
                }];
            } else {
                mpartwork = [[MPMediaItemArtwork alloc] initWithImage:self.artworkImage];
            }
            @try {
                currentlyPlayingTrackInfo[MPMediaItemPropertyArtwork] = mpartwork;
            } @catch (NSException *exception) {
                currentlyPlayingTrackInfo[MPMediaItemPropertyArtwork] = nil;
            }
        }
    #endif*/

        [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = currentlyPlayingTrackInfo;

}
