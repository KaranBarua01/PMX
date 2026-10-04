include(FetchContent)

# Pinned JUCE 9.0.3 release commit. Windows CI fetches this dependency.
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG be29c81492b6151c8ea8d14c840e1311963b3a83
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(JUCE)

option(PMX_ENABLE_NAM_CORE "Build NeuralAmpModelerCore support" ON)
if(PMX_ENABLE_NAM_CORE)
    FetchContent_Declare(
        NAMCore
        GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
        GIT_TAG 0b3d3c97b0859a3a8c92a8628c4dd89a25eb5842
        GIT_SHALLOW FALSE
        GIT_SUBMODULES_RECURSE TRUE
        SOURCE_SUBDIR cmake/pmx-no-upstream-add-subdirectory
    )
    FetchContent_MakeAvailable(NAMCore)
    file(GLOB NAM_CORE_SOURCES CONFIGURE_DEPENDS "${namcore_SOURCE_DIR}/NAM/*.cpp")
    file(GLOB NAM_CORE_WAVENET_SOURCES CONFIGURE_DEPENDS "${namcore_SOURCE_DIR}/NAM/*/*.cpp")
    # Every architecture registers itself in a static initializer. A normal archive
    # discards unreferenced model units and leaves get_dsp with an empty registry.
    add_library(pmx_nam_core OBJECT ${NAM_CORE_SOURCES} ${NAM_CORE_WAVENET_SOURCES})
    target_compile_features(pmx_nam_core PUBLIC cxx_std_20)
    target_include_directories(pmx_nam_core PUBLIC
        "${namcore_SOURCE_DIR}"
        "${namcore_SOURCE_DIR}/NAM"
        "${namcore_SOURCE_DIR}/Dependencies/eigen"
        "${namcore_SOURCE_DIR}/Dependencies/nlohmann"
        "${namcore_SOURCE_DIR}/Dependencies/AudioDSPTools/dsp"
    )
    target_compile_definitions(pmx_nam_core PUBLIC NAM_SAMPLE_FLOAT NAM_ENABLE_A2_FAST)
    if(WIN32)
        target_compile_definitions(pmx_nam_core PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    endif()
endif()
