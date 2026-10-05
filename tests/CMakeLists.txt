add_executable(pmx_version_tests version_tests.cpp)
target_link_libraries(pmx_version_tests PRIVATE pmx_build_config)
add_test(NAME pmx.version COMMAND pmx_version_tests)

add_executable(pmx_theme_tests ui/ThemeTests.cpp)
target_include_directories(pmx_theme_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(pmx_theme_tests PRIVATE cxx_std_20)
add_test(NAME pmx.theme COMMAND pmx_theme_tests)

add_executable(pmx_setup_state_tests state/SetupStateTests.cpp)
target_include_directories(pmx_setup_state_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(pmx_setup_state_tests PRIVATE cxx_std_20)
add_test(NAME pmx.setup_state COMMAND pmx_setup_state_tests)

add_executable(pmx_audio_device_tests audio/AudioDeviceControllerTests.cpp)
target_include_directories(pmx_audio_device_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_audio_device_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/audio/AudioDeviceController.cpp")
target_compile_features(pmx_audio_device_tests PRIVATE cxx_std_20)
add_test(NAME pmx.audio_device COMMAND pmx_audio_device_tests)

add_executable(pmx_safe_audio_tests audio/SafeAudioCallbackTests.cpp)
target_include_directories(pmx_safe_audio_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_safe_audio_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/audio/SafeAudioCallback.cpp")
target_compile_features(pmx_safe_audio_tests PRIVATE cxx_std_20)
add_test(NAME pmx.safe_audio COMMAND pmx_safe_audio_tests)

add_executable(pmx_processing_engine_tests audio/ProcessingEngineTests.cpp)
target_include_directories(pmx_processing_engine_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_processing_engine_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/audio/ProcessingEngine.cpp"
    "${PROJECT_SOURCE_DIR}/src/audio/SignalMetrics.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/OutputProtector.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/GuitarRack.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Gate.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Compressor.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Drive.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Eq.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Chorus.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Delay.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Reverb.cpp"
    "${PROJECT_SOURCE_DIR}/src/looper/LooperEngine.cpp"
    "${PROJECT_SOURCE_DIR}/src/recording/QuickRecorder.cpp"
    "${PROJECT_SOURCE_DIR}/src/nam/NamProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/src/ir/IrProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/src/audio/TempoService.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/Metronome.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/RhythmDrumMachine.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/GuitarDrumTrigger.cpp"
    "${PROJECT_SOURCE_DIR}/src/analysis/TunerTap.cpp")
target_compile_features(pmx_processing_engine_tests PRIVATE cxx_std_20)
add_test(NAME pmx.processing_engine COMMAND pmx_processing_engine_tests)

add_executable(pmx_output_protector_tests audio/OutputProtectorTests.cpp)
target_include_directories(pmx_output_protector_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_output_protector_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/dsp/OutputProtector.cpp")
target_compile_features(pmx_output_protector_tests PRIVATE cxx_std_20)
add_test(NAME pmx.output_protector COMMAND pmx_output_protector_tests)

add_executable(pmx_guitar_rack_tests dsp/GuitarRackTests.cpp)
target_include_directories(pmx_guitar_rack_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_guitar_rack_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/dsp/GuitarRack.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Gate.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Compressor.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Drive.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Eq.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Chorus.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Delay.cpp"
    "${PROJECT_SOURCE_DIR}/src/dsp/effects/Reverb.cpp")
target_compile_features(pmx_guitar_rack_tests PRIVATE cxx_std_20)
add_test(NAME pmx.guitar_rack COMMAND pmx_guitar_rack_tests)

add_executable(pmx_delay_tests dsp/DelayTests.cpp)
target_include_directories(pmx_delay_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_delay_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/dsp/effects/Delay.cpp")
target_compile_features(pmx_delay_tests PRIVATE cxx_std_20)
add_test(NAME pmx.delay COMMAND pmx_delay_tests)

add_executable(pmx_tuner_tests analysis/TunerEngineTests.cpp)
target_include_directories(pmx_tuner_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_tuner_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/analysis/TunerEngine.cpp")
target_compile_features(pmx_tuner_tests PRIVATE cxx_std_20)
add_test(NAME pmx.tuner COMMAND pmx_tuner_tests)

add_executable(pmx_tempo_tests audio/TempoServiceTests.cpp)
target_include_directories(pmx_tempo_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_tempo_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/audio/TempoService.cpp")
target_compile_features(pmx_tempo_tests PRIVATE cxx_std_20)
add_test(NAME pmx.tempo COMMAND pmx_tempo_tests)

add_executable(pmx_asset_library_tests assets/AssetLibraryTests.cpp)
target_include_directories(pmx_asset_library_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_asset_library_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/assets/AssetLibrary.cpp")
target_compile_features(pmx_asset_library_tests PRIVATE cxx_std_20)
add_test(NAME pmx.asset_library COMMAND pmx_asset_library_tests)

add_executable(pmx_ir_tests ir/IrProcessorTests.cpp)
target_include_directories(pmx_ir_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_ir_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/ir/IrProcessor.cpp")
target_compile_features(pmx_ir_tests PRIVATE cxx_std_20)
add_test(NAME pmx.ir COMMAND pmx_ir_tests)

add_executable(pmx_preset_store_tests presets/PresetStoreTests.cpp)
target_include_directories(pmx_preset_store_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_preset_store_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/presets/PresetStore.cpp")
target_compile_features(pmx_preset_store_tests PRIVATE cxx_std_20)
add_test(NAME pmx.preset_store COMMAND pmx_preset_store_tests)

add_executable(pmx_looper_tests looper/LooperEngineTests.cpp)
target_include_directories(pmx_looper_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_looper_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/looper/LooperEngine.cpp"
    "${PROJECT_SOURCE_DIR}/src/looper/LoopExportService.cpp")
target_compile_features(pmx_looper_tests PRIVATE cxx_std_20)
add_test(NAME pmx.looper COMMAND pmx_looper_tests)

add_executable(pmx_quick_recorder_tests recording/QuickRecorderTests.cpp)
target_include_directories(pmx_quick_recorder_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_quick_recorder_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/recording/QuickRecorder.cpp")
target_compile_features(pmx_quick_recorder_tests PRIVATE cxx_std_20)
add_test(NAME pmx.quick_recorder COMMAND pmx_quick_recorder_tests)

add_executable(pmx_audio_diagnostics_tests audio/AudioDiagnosticServiceTests.cpp)
target_include_directories(pmx_audio_diagnostics_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_audio_diagnostics_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/audio/AudioDiagnosticService.cpp")
target_compile_features(pmx_audio_diagnostics_tests PRIVATE cxx_std_20)
add_test(NAME pmx.audio_diagnostics COMMAND pmx_audio_diagnostics_tests)

add_executable(pmx_release_checker_tests update/ReleaseCheckerTests.cpp)
target_include_directories(pmx_release_checker_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_release_checker_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/update/ReleaseChecker.cpp")
target_compile_features(pmx_release_checker_tests PRIVATE cxx_std_20)
add_test(NAME pmx.release_checker COMMAND pmx_release_checker_tests)

add_executable(pmx_shortcut_tests input/ShortcutManagerTests.cpp)
target_include_directories(pmx_shortcut_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_shortcut_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/input/ShortcutManager.cpp")
target_compile_features(pmx_shortcut_tests PRIVATE cxx_std_20)
add_test(NAME pmx.shortcuts COMMAND pmx_shortcut_tests)

add_executable(pmx_layout_policy_tests ui/LayoutPolicyTests.cpp)
target_include_directories(pmx_layout_policy_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(pmx_layout_policy_tests PRIVATE cxx_std_20)
add_test(NAME pmx.layout_policy COMMAND pmx_layout_policy_tests)


add_executable(pmx_tuner_tap_tests analysis/TunerTapTests.cpp)
target_include_directories(pmx_tuner_tap_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_tuner_tap_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/analysis/TunerTap.cpp")
target_compile_features(pmx_tuner_tap_tests PRIVATE cxx_std_20)
add_test(NAME pmx.tuner_tap COMMAND pmx_tuner_tap_tests)

add_executable(pmx_runtime_policy_tests app/RuntimePolicyTests.cpp)
target_include_directories(pmx_runtime_policy_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_sources(pmx_runtime_policy_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/app/RuntimePolicy.cpp")
target_compile_features(pmx_runtime_policy_tests PRIVATE cxx_std_20)
add_test(NAME pmx.runtime_policy COMMAND pmx_runtime_policy_tests)

add_executable(pmx_effect_safety_tests dsp/EffectSafetyTests.cpp "${PROJECT_SOURCE_DIR}/src/dsp/effects/Eq.cpp" "${PROJECT_SOURCE_DIR}/src/dsp/effects/Gate.cpp" "${PROJECT_SOURCE_DIR}/src/dsp/effects/Compressor.cpp" "${PROJECT_SOURCE_DIR}/src/dsp/effects/Drive.cpp")
target_include_directories(pmx_effect_safety_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(pmx_effect_safety_tests PRIVATE cxx_std_20)
add_test(NAME pmx.effect_safety COMMAND pmx_effect_safety_tests)

add_executable(pmx_sound_state_tests presets/SoundStateTests.cpp "${PROJECT_SOURCE_DIR}/src/presets/PresetStore.cpp")
target_include_directories(pmx_sound_state_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(pmx_sound_state_tests PRIVATE cxx_std_20)
add_test(NAME pmx.sound_state COMMAND pmx_sound_state_tests)


add_executable(pmx_rhythm_drum_tests dsp/RhythmDrumMachineTests.cpp
    "${PROJECT_SOURCE_DIR}/src/dsp/RhythmDrumMachine.cpp"
    "${PROJECT_SOURCE_DIR}/src/audio/TempoService.cpp")
target_include_directories(pmx_rhythm_drum_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(pmx_rhythm_drum_tests PRIVATE cxx_std_20)
add_test(NAME pmx.rhythm_drums COMMAND pmx_rhythm_drum_tests)


add_executable(pmx_guitar_drum_trigger_tests dsp/GuitarDrumTriggerTests.cpp
    "${PROJECT_SOURCE_DIR}/src/dsp/GuitarDrumTrigger.cpp")
target_include_directories(pmx_guitar_drum_trigger_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(pmx_guitar_drum_trigger_tests PRIVATE cxx_std_20)
add_test(NAME pmx.guitar_drum_trigger COMMAND pmx_guitar_drum_trigger_tests)


add_executable(pmx_guitar_profile_tests instruments/GuitarProfileTests.cpp
    "${PROJECT_SOURCE_DIR}/src/instruments/GuitarProfile.cpp"
    "${PROJECT_SOURCE_DIR}/src/instruments/GuitarCalibration.cpp")
target_include_directories(pmx_guitar_profile_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(pmx_guitar_profile_tests PRIVATE cxx_std_20)
add_test(NAME pmx.guitar_profile COMMAND pmx_guitar_profile_tests)
