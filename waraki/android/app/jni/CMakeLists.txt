cmake_minimum_required(VERSION 3.6)

project(GAME)

# SDL sources are in a subfolder named "SDL"

set(SDLMIXER_VENDORED OFF)

add_subdirectory(../../../../thirdParty/SDL3-3.4.0 SDL3-build EXCLUDE_FROM_ALL)
add_subdirectory(../../../../thirdParty/SDL_mixer SDL_mixer-build EXCLUDE_FROM_ALL)

# Compilation of companion libraries
#add_subdirectory(SDL_image)
#add_subdirectory(SDL_mixer)
#add_subdirectory(SDL_ttf)

# Your game and its CMakeLists.txt are in a subfolder named "src"
add_subdirectory(src)

