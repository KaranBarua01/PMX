#pragma once
#include <array>
#include <functional>
#include <string>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"
#include "ui/components/PmxCard.h"
#include "ui/components/TunerView.h"
#include "ui/dialogs/EffectEditor.h"
#include "ui/dialogs/NamIrBrowser.h"

namespace pmx::ui
{
class LiveScreen final : public juce::Component
{
public:
    LiveScreen();
    void paint(juce::Graphics&) override;
    void resized() override;
    void toggleTuner();
    void setBypassVisual(bool enabled);
    void setStatusText(const std::string& text, bool healthy);
    void setPresetName(const std::string& name);
    void setNamName(const std::string& name);
    void setIrName(const std::string& name);
    void setTunerResult(const analysis::TunerResult& result) { tunerView.setResult(result); }
    [[nodiscard]] bool hasModalEditorOpen() const noexcept { return effectEditor.isVisible() || namIrBrowser.isVisible(); }
    [[nodiscard]] bool tunerVisible() const noexcept { return tunerView.isVisible(); }
    [[nodiscard]] float delayTimeMs() const noexcept { return effectEditor.timeMs(); }
    [[nodiscard]] float delayFeedbackPercent() const noexcept { return effectEditor.feedbackPercent(); }
    [[nodiscard]] float delayMixPercent() const noexcept { return effectEditor.mixPercent(); }

    std::function<void()> onChooseSound;
    std::function<void()> onPerformanceMode;
    std::function<void()> onSavePreset;
    std::function<void(bool)> onBypassChanged;
    std::function<void(float,float,float)> onDelayChanged;
    std::function<void()> onTapTempo;
    std::function<void()> onImportNam;
    std::function<void()> onImportIr;

private:
    juce::Label eyebrow,title,subtitle,status;
    PmxButton tuner{"TUNER"}; TunerView tunerView; PmxButton chooseSound{"CHOOSE A SOUND"}; PmxButton perform{"PERFORM"}; PmxButton save{"SAVE PRESET",ButtonKind::primary}; PmxButton bypass{"BYPASS"};
    std::array<PmxButton,9> modules { PmxButton{"GATE"},PmxButton{"COMP"},PmxButton{"DRIVE"},PmxButton{"NAM"},PmxButton{"IR"},PmxButton{"EQ"},PmxButton{"MOD"},PmxButton{"DELAY"},PmxButton{"REVERB"} };
    PmxButton namCard{"BE100-BE"}; PmxButton irCard{"24sVe 2×12"};
    EffectEditor effectEditor;
    NamIrBrowser namIrBrowser;
};
}
