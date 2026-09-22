cmake_minimum_required(VERSION 3.6)

project(main)

if(NOT TARGET SDL3::SDL3)
    find_package(SDL3 CONFIG REQUIRED)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_package(Vulkan REQUIRED)

find_package(sodium REQUIRED CONFIG)

#include_directories(${ANDROID_NDK}/sources/third_party/vulkan/src/include)
#include_directories(${ANDROID_NDK}/sources/third_party/vulkan/src/common)

#find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

include_directories(
    ../../../../../katipo/core
    ../../../../../katipo/thirdParty/tui/source
    ../../../../../katipo/thirdParty/enet-1.3.18/include
    ../../../../../katipo/thirdParty/liblmdb
    ../../../../../katipo/thirdParty/glm
    ../../../../../katipo/thirdParty/tui/thirdParty/glm/glm
    ../../../../../thirdParty/VulkanMemoryAllocator-3.2.1/include
    ../../../../../thirdParty/SDL3-3.4.0/include/SDL3
    ../../../../../thirdParty/SDL_mixer-release-3.2.4/include/SDL3_mixer
    ../../../../../thirdParty
    ../../../../../browser
    ../../../../
    ../../../../../thirdParty/utfcpp-3.2.1/source
    $ENV{VULKAN_SDK}/include
)

add_library(main SHARED
    main.cpp
    ../../../../../browser/Camera.cpp
    ../../../../../browser/BrowserHost.cpp
    ../../../../../browser/BrowserTracker.cpp
    ../../../../../browser/EventManager.cpp
    ../../../../../browser/GCommandBuffer.cpp
    ../../../../../browser/GPipeline.cpp
    ../../../../../browser/KatipoUtilities.cpp
    ../../../../../browser/MainController.cpp
    ../../../../../browser/MathUtils.cpp
    ../../../../../browser/MJAudio.cpp
    ../../../../../browser/MJAudioSDLMixer.cpp
    ../../../../../browser/MJCache.cpp
    ../../../../../browser/MJColorView.cpp
    ../../../../../browser/MJDataTexture.cpp
    ../../../../../browser/MJDrawable.cpp
    ../../../../../browser/MJDrawQuad.cpp
    ../../../../../browser/MJFont.cpp
    ../../../../../browser/MJImageTexture.cpp
    ../../../../../browser/MJImageView.cpp
    ../../../../../browser/MJRawImageTexture.cpp
    ../../../../../browser/MJRawImageTexture3D.cpp
    ../../../../../browser/MJRenderBuffer2.cpp
    ../../../../../browser/MJRenderBufferIndexed2.cpp
    ../../../../../browser/MJRenderTarget.cpp
    ../../../../../browser/MJRenderTarget3D.cpp
    ../../../../../browser/MJRenderTargetAccumulation.cpp
    ../../../../../browser/MJRenderTargetShadow.cpp
    ../../../../../browser/MJRenderTargetSSAO.cpp
    ../../../../../browser/MJStaticRenderBuffer.cpp
    ../../../../../browser/MJStaticRenderBuffer2.cpp
    ../../../../../browser/MJTextView.cpp
    ../../../../../browser/MJTimer.cpp
    ../../../../../browser/MJView.cpp
    ../../../../../browser/Noise3DFast.cpp
    ../../../../../browser/Vulkan.cpp
    ../../../../../browser/VulkanMemoryThread.cpp
    ../../../../../katipo/core/ClientNetInterface.cpp
    ../../../../../katipo/core/Database.cpp
    ../../../../../katipo/core/DatabaseEnvironment.cpp
    ../../../../../katipo/core/NetServerClient.cpp
    ../../../../../katipo/core/Server.cpp
    ../../../../../katipo/core/ServerNetInterface.cpp
    ../../../../../katipo/core/Timer.cpp
    ../../../../../katipo/core/Tracker.cpp
    ../../../../../katipo/core/Scanner.cpp
    ../../../../../katipo/thirdParty/enet-1.3.18/callbacks.c
    ../../../../../katipo/thirdParty/enet-1.3.18/compress.c
    ../../../../../katipo/thirdParty/enet-1.3.18/host.c
    ../../../../../katipo/thirdParty/enet-1.3.18/list.c
    ../../../../../katipo/thirdParty/enet-1.3.18/packet.c
    ../../../../../katipo/thirdParty/enet-1.3.18/peer.c
    ../../../../../katipo/thirdParty/enet-1.3.18/protocol.c
    ../../../../../katipo/thirdParty/enet-1.3.18/unix.c
    ../../../../../katipo/thirdParty/liblmdb/mdb.c
    ../../../../../katipo/thirdParty/liblmdb/midl.c
    ../../../../../katipo/thirdParty/tui/source/TuiBuiltInFunctions.cpp
    ../../../../../katipo/thirdParty/tui/source/TuiFileUtils.cpp
    ../../../../../katipo/thirdParty/tui/source/TuiFunction.cpp
    ../../../../../katipo/thirdParty/tui/source/TuiRef.cpp
    ../../../../../katipo/thirdParty/tui/source/TuiSha1.cpp
    ../../../../../katipo/thirdParty/tui/source/TuiTable.cpp
    ../../../../../thirdParty/MurmurHash3.cpp
    ../../../../Waraki.cpp
    #${ANDROID_NDK}/sources/third_party/vulkan/src/common/vulkan_wrapper.cpp
)



target_link_libraries(main PRIVATE Threads::Threads)
target_link_libraries(main PRIVATE vulkan)
target_link_libraries(main PRIVATE SDL3::SDL3)
target_link_libraries(main PRIVATE sodium::sodium)
target_link_libraries(main PRIVATE SDL3_mixer::SDL3_mixer)
