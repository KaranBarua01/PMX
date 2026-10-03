#pragma once
#include <array>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"
namespace pmx::ui { class NamIrBrowser final: public juce::Component { public: NamIrBrowser(); void paint(juce::Graphics&) override; void resized() override; std::function<void()> onImportNam,onImportIr,onLoadSelected; private: juce::Label title,subtitle,ampLabel,cabLabel; std::array<PmxButton,4> amps{PmxButton{"BE100-BE"},PmxButton{"SPITFINE"},PmxButton{"DC30 ch2 b"},PmxButton{"MesaBoogi"}}; std::array<PmxButton,4> cabs{PmxButton{"24sVe 2×12"},PmxButton{"Blue M160"},PmxButton{"1966 Delux"},PmxButton{"O412st-v"}}; PmxButton importNam{"+ IMPORT NAM"},importIr{"+ IMPORT IR"},load{"LOAD SELECTED PAIR",ButtonKind::primary}; }; }
