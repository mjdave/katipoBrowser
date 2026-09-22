if(NOT TARGET sodium::sodium)
add_library(sodium::sodium SHARED IMPORTED)
set_target_properties(sodium::sodium PROPERTIES
    IMPORTED_LOCATION "/Users/davidframpton/.gradle/caches/8.14.5/transforms/045d2d38be842db6af593f661987f76b/transformed/libsodium-1.0.22.0/prefab/modules/sodium/libs/android.arm64-v8a/libsodium.so"
    INTERFACE_INCLUDE_DIRECTORIES "/Users/davidframpton/.gradle/caches/8.14.5/transforms/045d2d38be842db6af593f661987f76b/transformed/libsodium-1.0.22.0/prefab/modules/sodium/libs/android.arm64-v8a/include"
    INTERFACE_LINK_LIBRARIES ""
)
endif()

if(NOT TARGET sodium::sodium-minimal)
add_library(sodium::sodium-minimal SHARED IMPORTED)
set_target_properties(sodium::sodium-minimal PROPERTIES
    IMPORTED_LOCATION "/Users/davidframpton/.gradle/caches/8.14.5/transforms/045d2d38be842db6af593f661987f76b/transformed/libsodium-1.0.22.0/prefab/modules/sodium-minimal/libs/android.arm64-v8a/libsodium.so"
    INTERFACE_INCLUDE_DIRECTORIES "/Users/davidframpton/.gradle/caches/8.14.5/transforms/045d2d38be842db6af593f661987f76b/transformed/libsodium-1.0.22.0/prefab/modules/sodium-minimal/libs/android.arm64-v8a/include"
    INTERFACE_LINK_LIBRARIES ""
)
endif()

if(NOT TARGET sodium::sodium-minimal-static)
add_library(sodium::sodium-minimal-static STATIC IMPORTED)
set_target_properties(sodium::sodium-minimal-static PROPERTIES
    IMPORTED_LOCATION "/Users/davidframpton/.gradle/caches/8.14.5/transforms/045d2d38be842db6af593f661987f76b/transformed/libsodium-1.0.22.0/prefab/modules/sodium-minimal-static/libs/android.arm64-v8a/libsodium-minimal-static.a"
    INTERFACE_INCLUDE_DIRECTORIES "/Users/davidframpton/.gradle/caches/8.14.5/transforms/045d2d38be842db6af593f661987f76b/transformed/libsodium-1.0.22.0/prefab/modules/sodium-minimal-static/libs/android.arm64-v8a/include"
    INTERFACE_LINK_LIBRARIES ""
)
endif()

if(NOT TARGET sodium::sodium-static)
add_library(sodium::sodium-static STATIC IMPORTED)
set_target_properties(sodium::sodium-static PROPERTIES
    IMPORTED_LOCATION "/Users/davidframpton/.gradle/caches/8.14.5/transforms/045d2d38be842db6af593f661987f76b/transformed/libsodium-1.0.22.0/prefab/modules/sodium-static/libs/android.arm64-v8a/libsodium-static.a"
    INTERFACE_INCLUDE_DIRECTORIES "/Users/davidframpton/.gradle/caches/8.14.5/transforms/045d2d38be842db6af593f661987f76b/transformed/libsodium-1.0.22.0/prefab/modules/sodium-static/libs/android.arm64-v8a/include"
    INTERFACE_LINK_LIBRARIES ""
)
endif()

