#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "analysis/TunerEngine.h"
namespace pmx::ui { class TunerView final: public juce::Component { public: void setResult(const analysis::TunerResult&); void paint(juce::Graphics&) override; private: analysis::TunerResult result; }; }
