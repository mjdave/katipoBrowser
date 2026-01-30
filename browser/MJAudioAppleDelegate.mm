//
//  MJAudio.cpp
//  Ambience
//
//  Created by David Frampton on 8/06/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#include "MJAudioAppleDelegate.h"
#import <AVFoundation/AVFoundation.h>
#include "MJAudioApple.h"
#include "MJAudio.h"

@implementation MJAudioAppleDelegate


- (id)init
{
#if TARGET_OS_IPHONE
    NSError *error;
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&error];
#endif
    
    player = [[AVQueuePlayer alloc] init];
    return self;
}

- (void)updateInfo:(AVPlayerItem*)playerItem
{
    AVAsset* asset = [playerItem asset];
    
    NSArray* commonMetaData = [asset commonMetadata];
    
    std::string titleString;
    std::string artistString;
    NSData* artImageData = nullptr;
    double duration = CMTimeGetSeconds([asset duration]);
    
    NSArray* titleItems = [AVMetadataItem metadataItemsFromArray:commonMetaData filteredByIdentifier:AVMetadataCommonIdentifierTitle];
    if([titleItems count] > 0)
    {
        titleString = [[[titleItems objectAtIndex:0] stringValue] UTF8String];
    }
    NSArray* artistItems = [AVMetadataItem metadataItemsFromArray:commonMetaData filteredByIdentifier:AVMetadataCommonIdentifierArtist];
    if([artistItems count] > 0)
    {
        artistString = [[[artistItems objectAtIndex:0] stringValue] UTF8String];
    }
    
    NSArray* artItems = [AVMetadataItem metadataItemsFromArray:commonMetaData filteredByIdentifier:AVMetadataCommonIdentifierArtwork];
    if([artItems count] > 0)
    {
        artImageData = [[artItems objectAtIndex:0] dataValue];
    }
    
    /*for(AVMetadataItem* item in commonMetaData) //doesn't work because this is the worst api ever designed wtf apple
    {
        if(item.identifier == AVMetadataCommonIdentifierTitle)
        {
            titleString = [[item stringValue] UTF8String];
        }
        else if(item.identifier == AVMetadataCommonIdentifierArtist)
        {
            artistString = [[item stringValue] UTF8String];
        }
    }*/
    
    
    MJAudio::getInstance()->updateCurrentlyPlayingInfo(titleString, artistString, duration, (void*)artImageData.bytes, artImageData.length);
    
    //NSArray* titleItems = [AVMetadataItem metadataItemsFromArray:commonMetaData filteredByIdentifier:AVMetadataCommonIdentifierTitle];
    
    /*if([titleItems count] > 0)
    {
        NSString* titleString = [[titleItems objectAtIndex:0] stringValue];
        
        MJAudio::getInstance()->updateCurrentlyPlayingTrackInfo(titleString.UTF8String);
    }*/
}


- (void)playerItemDidReachEnd:(NSNotification *)notification {
    MJLog("got playerItemDidReachEnd notifcation callback")
//song reached end of playing - respond appropriately
    /*AVPlayerItem* currentItem = [notification object];//[player currentItem];
    
    AVAsset* asset = [currentItem asset];
    
    NSArray* commonMetaData = [asset commonMetadata];
    NSArray* titleItems = [AVMetadataItem metadataItemsFromArray:commonMetaData filteredByIdentifier:AVMetadataCommonIdentifierTitle];
    
    int queueIndex = queueIndexesByPlayerItems[currentItem];
    if(urlsByQueueIndex.count(queueIndex + 1) != 0)
    {
        const std::string& nextUrlString = urlsByQueueIndex[queueIndex + 1];
        MJAudio::getInstance()->updateCurrentlyPlayingTrackInfo(Tui::fileNameFromPath(nextUrlString));
    }*/
    
    AVPlayerItem* nextItem = nullptr;
    if(nextPlayerItemByPlayerItem.count(player.currentItem) != 0)
    {
        nextItem = nextPlayerItemByPlayerItem[player.currentItem];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(playerItemDidReachEnd:)
                                                     name:AVPlayerItemDidPlayToEndTimeNotification
                                                   object:nextItem];
    }
    [self updateInfo:nextItem];
}

- (void)sendUpdatedPausedState
{
    MJAudio::getInstance()->updatePausedState([player timeControlStatus] == AVPlayerTimeControlStatusPaused);
}

- (bool)paused
{
    return [player timeControlStatus] == AVPlayerTimeControlStatusPaused;
}


- (void)play:(TuiTable*)urls
{
    if(urls)
    {
        [player removeAllItems];
        
        //std::map<AVPlayerItem*, int> queueIndexesByPlayerItems;
        //std::map<int, std::string> urlsByQueueIndex;
        
        nextPlayerItemByPlayerItem.clear();
       // urlsByQueueIndex.clear();
        
        int index = 0;
        AVPlayerItem* prevPlayerItem = nullptr;
        for(TuiRef* urlString : urls->arrayObjects)
        {
            std::string path = ((TuiString*)urlString)->value;
            AVPlayerItem* playerItem = [[AVPlayerItem alloc] initWithURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]]];//((TuiString*)urlString)->value.c_str()]]];
            [player insertItem:playerItem afterItem:nil];
            
            if(prevPlayerItem)
            {
                nextPlayerItemByPlayerItem[prevPlayerItem] = playerItem;
            }
            prevPlayerItem = playerItem;
            
            index++;
            hasUrls = true;
        }
        
        player.actionAtItemEnd = AVPlayerActionAtItemEndAdvance;
        
    }
    
    
    
    //player.volume = 1.0;
    //player.delegate = self;
    
    if(hasUrls)
    {
        [player play];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(playerItemDidReachEnd:)
                                                     name:AVPlayerItemDidPlayToEndTimeNotification
                                                   object:[player currentItem]];
        
        
        [self updateInfo:[player currentItem]];
        [self sendUpdatedPausedState];
    }
    //const std::string& nextUrlString = urlsByQueueIndex[0];
    //MJAudio::getInstance()->updateCurrentlyPlayingTrackName(Tui::fileNameFromPath(nextUrlString));
}

-(void)stop
{
    [player pause];
}


-(void)skipToNextTrack
{
    [player advanceToNextItem];
    [self updateInfo:[player currentItem]];
}



- (MPRemoteCommandHandlerStatus )remoteCommandEvent:(MPRemoteCommandEvent *)event
{
    MPRemoteCommandCenter *cc = [MPRemoteCommandCenter sharedCommandCenter];

    if (event.command == cc.pauseCommand) {
        [player pause];
        [self sendUpdatedPausedState];
        return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.playCommand) {
        [player play];
        [self sendUpdatedPausedState];
        return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.stopCommand) {
        [player pause];
        [self sendUpdatedPausedState];
        return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.togglePlayPauseCommand) {
       // [vps playPause];
        if([player timeControlStatus] == AVPlayerTimeControlStatusPaused)
        {
            [player play];
        }
        else
        {
            [player pause];
        }
        [self sendUpdatedPausedState];
        return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.nextTrackCommand) {
        [player advanceToNextItem];
        [self updateInfo:[player currentItem]];
        return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.previousTrackCommand) {
        [player seekToTime:CMTimeMake(0, NSEC_PER_SEC)];
        return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.skipForwardCommand) {
        return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.skipBackwardCommand) {
        return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.changePlaybackPositionCommand) {
        /*MPChangePlaybackPositionCommandEvent *positionEvent = (MPChangePlaybackPositionCommandEvent *)event;
        NSInteger duration = vps.mediaDuration / 1000;
        if (duration > 0) {
            vps.playbackPosition = positionEvent.positionTime / duration;
            return MPRemoteCommandHandlerStatusSuccess;
        }
        return MPRemoteCommandHandlerStatusCommandFailed;*/
        
         return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.changeShuffleModeCommand) {
        //MPChangeShuffleModeCommandEvent *shuffleEvent = (MPChangeShuffleModeCommandEvent *)event;
        //vps.shuffleMode = shuffleEvent.shuffleType != MPShuffleTypeOff;
        return MPRemoteCommandHandlerStatusSuccess;
    }
    if (event.command == cc.changeRepeatModeCommand) {
       /* MPChangeRepeatModeCommandEvent *repeatEvent = (MPChangeRepeatModeCommandEvent *)event;
        switch (repeatEvent.repeatType) {
            case MPRepeatTypeOne:
                vps.repeatMode = VLCRepeatCurrentItem;
                break;

            case MPRepeatTypeAll:
                vps.repeatMode = VLCRepeatAllItems;
                break;

            default:
                vps.repeatMode = VLCDoNotRepeat;
                break;
        }*/
        return MPRemoteCommandHandlerStatusSuccess;
    }
    return MPRemoteCommandHandlerStatusCommandFailed;

}


@end
