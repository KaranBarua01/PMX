#pragma once
#include <array>
#include <functional>
#include <string>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"
#include "ui/components/TunerView.h"
#include "ui/dialogs/EffectEditor.h"
#include "ui/dialogs/NamIrBrowser.h"
#include "presets/SoundState.h"
namespace pmx::ui {
class LiveScreen final:public juce::Component {
public:
 LiveScreen();void paint(juce::Graphics&) override;void resized() override;
 void toggleTuner();void setBypassVisual(bool);void setMutedVisual(bool);void setRecordingVisual(bool);
 void editEffect(std::size_t i){openEditor(i);}
 void setStatusText(const std::string&,bool);void setPresetName(const std::string&);void setNamName(const std::string&);void setIrName(const std::string&);
 void setSoundState(const presets::SoundState&);void setMeters(float,float);void setTempo(double);
 void setTunerResult(const analysis::TunerResult& r){tunerView.setResult(r);}
 bool hasModalEditorOpen() const noexcept{return effectEditor.isVisible()||namIrBrowser.isVisible();}
 bool tunerVisible() const noexcept{return tunerView.isVisible();}
 float delayTimeMs() const noexcept{return sound.effects[7].values[0];}
 float delayFeedbackPercent() const noexcept{return sound.effects[7].values[1];}
 float delayMixPercent() const noexcept{return sound.effects[7].values[2];}
 std::function<void()> onChooseSound,onPerformanceMode,onSavePreset,onTapTempo,onImportNam,onImportIr,onQuickRecord,onOpenLooper;
 std::function<void(bool)> onBypassChanged,onMuteChanged,onMetronomeChanged;
 std::function<void(float)> onMetronomeLevel;
 std::function<void(double)> onTempoChanged;
 std::function<void(std::size_t,const presets::EffectSettings&)> onEffectChanged;
 std::function<void(float,float,float)> onDelayChanged;
private:
 void openEditor(std::size_t);void openLibrary();
 presets::SoundState sound;float inputPeak{},outputPeak{};
 juce::Rectangle<int> soundCard;juce::Label eyebrow,title,subtitle,status,mode,tempoLabel,clickLevelLabel;
 PmxButton tuner{"TUNER"},chooseSound{"CHOOSE A SOUND"},perform{"PERFORM"},save{"SAVE PRESET",ButtonKind::primary},bypass{"BYPASS"},mute{"UNMUTE"},record{"QUICK RECORD"},click{"CLICK OFF"},tap{"TAP TEMPO"},openLooper{"OPEN LOOPER"};
 juce::Slider bpm,clickLevel;TunerView tunerView;
 std::array<PmxButton,9> modules{PmxButton{"GATE"},PmxButton{"COMP"},PmxButton{"DRIVE"},PmxButton{"NAM"},PmxButton{"IR"},PmxButton{"EQ"},PmxButton{"MOD"},PmxButton{"DELAY"},PmxButton{"REVERB"}};
 PmxButton namCard{"IMPORT AN AMP"},irCard{"IMPORT A CABINET"};EffectEditor effectEditor;NamIrBrowser namIrBrowser;
};}

