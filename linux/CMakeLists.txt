cmake_minimum_required(VERSION 3.7)

project(katipoBrowser VERSION 0.3)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

IF(APPLE)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF()

#set(GCC_COVERAGE_LINK_FLAGS    "-no-pie")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags,-rpath='$ORIGIN/lib'")
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(SOURCES 
../browser/Camera.cpp
../browser/EventManager.cpp
../browser/GCommandBuffer.cpp
../browser/GPipeline.cpp
../browser/KatipoBrowser.cpp
../browser/KatipoUtilities.cpp
../browser/MainController.cpp
../browser/MathUtils.cpp
../browser/MJAudio.cpp
../browser/MJAudioSDLMixer.cpp
../browser/MJCache.cpp
../browser/MJColorView.cpp
../browser/MJDataTexture.cpp
../browser/MJDrawable.cpp
../browser/MJDrawQuad.cpp
../browser/MJFont.cpp
../browser/MJImageTexture.cpp
../browser/MJImageView.cpp
../browser/MJRawImageTexture.cpp
../browser/MJRawImageTexture3D.cpp
../browser/MJRenderBuffer2.cpp
../browser/MJRenderBufferIndexed2.cpp
../browser/MJRenderTarget.cpp
../browser/MJRenderTarget3D.cpp
../browser/MJRenderTargetAccumulation.cpp
../browser/MJRenderTargetShadow.cpp
../browser/MJRenderTargetSSAO.cpp
../browser/MJStaticRenderBuffer.cpp
../browser/MJStaticRenderBuffer2.cpp
../browser/MJTextView.cpp
../browser/MJTimer.cpp
../browser/MJView.cpp
../browser/Noise3DFast.cpp
../browser/Vulkan.cpp
../browser/VulkanMemoryThread.cpp
../katipo/core/ClientNetInterface.cpp
../katipo/core/Database.cpp
../katipo/core/DatabaseEnvironment.cpp
../katipo/core/NetServerClient.cpp
../katipo/core/Server.cpp
../katipo/core/ServerNetInterface.cpp
../katipo/core/Timer.cpp
../katipo/thirdParty/enet-1.3.18/callbacks.c
../katipo/thirdParty/enet-1.3.18/compress.c
../katipo/thirdParty/enet-1.3.18/host.c
../katipo/thirdParty/enet-1.3.18/list.c
../katipo/thirdParty/enet-1.3.18/packet.c
../katipo/thirdParty/enet-1.3.18/peer.c
../katipo/thirdParty/enet-1.3.18/protocol.c
../katipo/thirdParty/enet-1.3.18/unix.c
../katipo/thirdParty/liblmdb/mdb.c
../katipo/thirdParty/liblmdb/midl.c
../katipo/thirdParty/tui/source/TuiBuiltInFunctions.cpp
../katipo/thirdParty/tui/source/TuiFileUtils.cpp
../katipo/thirdParty/tui/source/TuiFunction.cpp
../katipo/thirdParty/tui/source/TuiRef.cpp
../katipo/thirdParty/tui/source/TuiSha1.cpp
../katipo/thirdParty/tui/source/TuiTable.cpp
../thirdParty/MurmurHash3.cpp
main.cpp
)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(../thirdParty/SDL3-3.4.0 SDL3-build EXCLUDE_FROM_ALL)
add_subdirectory(../thirdParty/SDL_mixer-main SDL_mixer-build EXCLUDE_FROM_ALL)

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-deprecated")
endif ( CMAKE_COMPILER_IS_GNUCC )

include_directories(
    ../katipo/core
    ../katipo/thirdParty/tui/source
    ../katipo/thirdParty/enet-1.3.18/include
    ../katipo/thirdParty/liblmdb
    ../katipo/thirdParty/glm
    ../katipo/thirdParty/tui/thirdParty/glm/glm
    ../thirdParty/VulkanMemoryAllocator-3.2.1/include
    ../thirdParty/SDL3-3.4.0/include/SDL3
    ../thirdParty/SDL_mixer-main/include/SDL3_mixer
    ../thirdParty
    ../browser
    ../thirdParty/utfcpp-3.2.1/source
    $ENV{VULKAN_SDK}/include
)


find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

add_executable(katipoBrowser ${SOURCES})

if(APPLE)
    include_directories(../katipo/thirdParty/libsodium/macos/include)
    target_link_libraries(katipoBrowser ${CMAKE_SOURCE_DIR}/../katipo/thirdParty/libsodium/macos/lib/libsodium.a)
elseif(WIN32)
    include_directories(../katipo/thirdParty/libsodium/windows/include)
    #target_link_libraries(katipoBrowser ${CMAKE_SOURCE_DIR}/../katipo/thirdParty/libsodium/windows/lib/libsodium.a) #hmm
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    #todo
else()
    include_directories(../katipo/thirdParty/libsodium/linux/include)
    target_link_libraries(katipoBrowser ${CMAKE_SOURCE_DIR}/../katipo/thirdParty/libsodium/linux/lib/libsodium.a)
endif()

target_link_libraries(katipoBrowser Threads::Threads)
target_link_libraries(katipoBrowser ${Vulkan_LIBRARY})
target_link_libraries(katipoBrowser SDL3::SDL3)
target_link_libraries(katipoBrowser SDL3_mixer::SDL3_mixer)

set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
