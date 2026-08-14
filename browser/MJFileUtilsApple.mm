
#include "MJFileUtilsApple.h"
#include "MJLog.h"
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

std::string createSecurityBookmarkForDirectory(std::string path)
{
    if(path.empty())
    {
        return "";
    }

    NSString *pathString = [NSString stringWithUTF8String:path.c_str()];
    NSURL *url = [NSURL fileURLWithPath:pathString isDirectory:YES];

    NSError *error = nil;
    NSData *bookmarkData =
        [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
          includingResourceValuesForKeys:nil
                           relativeToURL:nil
                                   error:&error];
    if(!bookmarkData)
    {
        MJWarn("Failed to create bookmark for %s: %s",
              path.c_str(), error.description.UTF8String);
        return "";
    }

    NSString *base64 = [bookmarkData base64EncodedStringWithOptions:0];

    return [base64 UTF8String];
}


std::string resolveSecurityBookmark(std::string bookmarkData)
{
    if(bookmarkData.empty())
    {
        return "";
    }

    NSString *base64String = [NSString stringWithUTF8String:bookmarkData.c_str()];
    NSData *bookmarkNSData = [[NSData alloc] initWithBase64EncodedString:base64String options:0];
    if (!bookmarkNSData) {
        MJWarn("Failed to decode bookmark data");
        return "";
    }

    BOOL isStale = NO;
    NSError *error = nil;
    NSURL *resolvedURL =
        [NSURL URLByResolvingBookmarkData:bookmarkNSData
                                  options:NSURLBookmarkResolutionWithSecurityScope
                            relativeToURL:nil
                      bookmarkDataIsStale:&isStale
                                    error:&error];
    if(!resolvedURL)
    {
        MJWarn("Failed to resolve security bookmark: %s", error.description.UTF8String);
        return "";
    }

    if(isStale)
    {
        MJWarn("Security bookmark is stale — the app may need to re-select the folder to refresh it");
    }

    BOOL started = [resolvedURL startAccessingSecurityScopedResource];
    if (!started) {
        MJWarn("startAccessingSecurityScopedResource failed for %s",
              [[resolvedURL path] UTF8String]);
    }

    return [[resolvedURL path] UTF8String];
}
