#pragma once
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <juce_gui_extra/juce_gui_extra.h>
#include "analysis/TunerEngine.h"
#include "audio/AudioDeviceController.h"
#include "audio/AudioDiagnosticService.h"
#include "audio/JuceAudioHost.h"
#include "audio/ProcessingEngine.h"
#include "instruments/GuitarCalibration.h"
#include "instruments/GuitarProfile.h"
#include "looper/LoopExportService.h"
#include "presets/PresetStore.h"
#include "presets/SoundState.h"
#include "ui/AppShell.h"

namespace pmx::app
{
class PmxRuntime final : private juce::Timer
{
public:
    explicit PmxRuntime(ui::AppShell& shell);
    ~PmxRuntime() override;

private:
    void timerCallback() override;
    void wireUi();
    void applyEffect(std::size_t,const presets::EffectSettings&);
    void refreshPresets();
    void loopCommand(audio::LoopCommand);
    looper::LoopSnapshot snapshotLoop();
    void discoverAndOpen();
    bool applySelection(const audio::AudioDeviceSelection&, bool enableMonitoring);
    void enableMonitoring();
    void updateDiagnostics();
    void updateLooperUi();
    void setSpecialInstrumentMode(ui::SpecialScreen::InstrumentMode mode);
    void handleShortcut(input::ShortcutCommand);
    void tapTempo();
    void chooseNam();
    void chooseIr();
    void loadNam(const std::filesystem::path&);
    void loadIr(const std::filesystem::path&);
    void commitPreset(const presets::Preset&);
    void saveCurrentPreset();
    void choosePreset(const std::string& name);
    void applyPreset(const presets::Preset&);
    void saveLoopToLibrary();
    void exportLoop();
    void toggleQuickRecord();
    void installUpdate(const std::filesystem::path&);
    [[nodiscard]] std::filesystem::path appDataDirectory() const;
    [[nodiscard]] static std::string slugify(const std::string&);
    [[nodiscard]] static juce::String looperStateText(looper::LooperState);

    ui::AppShell& shell;
    audio::JuceAudioHost audioHost;
    audio::AudioDeviceController deviceController;
    audio::ProcessingEngine engine;
    analysis::TunerEngine tunerEngine;
    instruments::GuitarProfile guitarProfile;
    instruments::GuitarCalibration guitarCalibration;
    presets::PresetStore presetStore;
    std::optional<audio::AudioDeviceSelection> currentSelection;
    std::unique_ptr<juce::FileChooser> chooser;
    std::future<looper::LoopExportResult> loopWrite;
    struct PreparedAsset {std::unique_ptr<nam::NamProcessor> nam;std::unique_ptr<ir::IrProcessor> ir;std::filesystem::path path;std::optional<presets::Preset> preset;std::string error;double rate{};int block{};};
    std::future<PreparedAsset> assetLoad;
    presets::SoundState sound;
    std::vector<presets::Preset> presetLibrary;
    std::vector<float> tunerWindow;
    std::filesystem::path currentNamPath;
    std::filesystem::path currentIrPath;
    std::string currentPresetName { "Dream Clean" };
    std::string currentPresetId{"factory-4"},currentCategory{"Clean"};
    bool connected { false };
    bool monitoring { false };
    bool bypassed { false };
    int diagnosticsCountdown { 0 };
};
} // namespace pmx::app
