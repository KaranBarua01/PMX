cmake_minimum_required(VERSION 3.24)

project(PMX VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PMX_VERSION_STRING "0.2.0-alpha.1" CACHE STRING "PMX semantic version")
option(PMX_BUILD_TESTS "Build dependency-free PMX tests" ON)
if(WIN32)
    option(PMX_BUILD_GUI "Build the native JUCE PMX application" ON)
else()
    option(PMX_BUILD_GUI "Build the native JUCE PMX application" OFF)
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/AppVersion.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/pmx/AppVersion.h"
    @ONLY
)

add_library(pmx_build_config INTERFACE)
target_include_directories(pmx_build_config INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_features(pmx_build_config INTERFACE cxx_std_20)

if(PMX_BUILD_GUI)
    include(cmake/Dependencies.cmake)

    juce_add_gui_app(PMX
        PRODUCT_NAME "PMX"
        COMPANY_NAME "PMX"
        BUNDLE_ID "com.pmx.audio"
    )

    target_sources(PMX PRIVATE
        src/app/Main.cpp
        src/app/PmxRuntime.cpp
        src/app/RuntimePolicy.cpp
        src/ui/PmxTheme.cpp
        src/ui/MainWindow.cpp
        src/ui/AppShell.cpp
        src/ui/components/PmxButton.cpp
        src/ui/components/PmxCard.cpp
        src/ui/components/PmxMeter.cpp
        src/ui/components/PmxKnob.cpp
        src/ui/screens/FirstRunWizard.cpp
        src/audio/AudioDeviceController.cpp
        src/audio/SafeAudioCallback.cpp
        src/audio/JuceAudioHost.cpp
        src/audio/ProcessingEngine.cpp
        src/audio/SignalMetrics.cpp
        src/dsp/OutputProtector.cpp
        src/dsp/GuitarRack.cpp
        src/dsp/effects/Gate.cpp
        src/dsp/effects/Compressor.cpp
        src/dsp/effects/Drive.cpp
        src/dsp/effects/Eq.cpp
        src/dsp/effects/Chorus.cpp
        src/dsp/effects/Delay.cpp
        src/dsp/effects/Reverb.cpp
        src/ui/screens/LiveScreen.cpp
        src/ui/dialogs/EffectEditor.cpp
        src/analysis/TunerEngine.cpp
        src/analysis/TunerTap.cpp
        src/audio/TempoService.cpp
        src/dsp/Metronome.cpp
        src/ui/components/TunerView.cpp
        src/assets/AssetLibrary.cpp
        src/nam/NamProcessor.cpp
        src/ir/IrProcessor.cpp
        src/ui/dialogs/NamIrBrowser.cpp
        src/presets/PresetStore.cpp
        src/ui/screens/PresetsScreen.cpp
        src/looper/LooperEngine.cpp
        src/looper/LoopExportService.cpp
        src/ui/screens/LooperScreen.cpp
        src/recording/QuickRecorder.cpp
        src/audio/AudioDiagnosticService.cpp
        src/ui/screens/SettingsScreen.cpp
        src/update/ReleaseChecker.cpp
        src/update/JuceReleaseClient.cpp
        src/ui/dialogs/UpdateDialog.cpp
        src/input/ShortcutManager.cpp
        src/ui/screens/PerformanceMode.cpp
    )
    target_include_directories(PMX PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(PMX PRIVATE
        pmx_build_config
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    )
    if(TARGET pmx_nam_core)
        target_link_libraries(PMX PRIVATE pmx_nam_core)
        target_compile_definitions(PMX PRIVATE PMX_HAS_NAM_CORE=1)
    endif()
    target_compile_definitions(PMX PRIVATE
        JUCE_ASIO=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
    if(PMX_BUILD_TESTS)
        juce_add_console_app(pmx_native_ui_tests PRODUCT_NAME "PMX native UI tests")
        get_target_property(PMX_UI_TEST_CANDIDATES PMX SOURCES)
        foreach(source IN LISTS PMX_UI_TEST_CANDIDATES)
            if(source MATCHES "^src/ui/.*\\.cpp$")
                target_sources(pmx_native_ui_tests PRIVATE "${source}")
            endif()
        endforeach()
        target_sources(pmx_native_ui_tests PRIVATE tests/ui/NativeUiTests.cpp src/input/ShortcutManager.cpp src/update/ReleaseChecker.cpp src/update/JuceReleaseClient.cpp)
        target_include_directories(pmx_native_ui_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
        target_link_libraries(pmx_native_ui_tests PRIVATE pmx_build_config juce::juce_gui_extra juce::juce_audio_utils)
        target_compile_definitions(pmx_native_ui_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    endif()
endif()

if(PMX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    if(TARGET pmx_native_ui_tests)
        add_test(NAME pmx.native_ui COMMAND pmx_native_ui_tests "${CMAKE_BINARY_DIR}/ui-renders")
    endif()
endif()
