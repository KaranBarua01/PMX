#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"
namespace pmx::ui {
class NamIrBrowser final:public juce::Component {
public:
 NamIrBrowser();void paint(juce::Graphics&) override;void resized() override;
 void setNamName(const std::string& s){amp.setButtonText(juce::String(s));amp.getProperties().set("subtitle","CURRENT AMP");amp.repaint();}
 void setIrName(const std::string& s){cab.setButtonText(juce::String(s));cab.getProperties().set("subtitle","CURRENT CABINET");cab.repaint();}
 std::function<void()> onImportNam,onImportIr,onLoadSelected;
private:
 juce::Rectangle<int> card;juce::Label title,subtitle,ampLabel,cabLabel,hint;
 PmxButton amp{"No amp imported"},cab{"No cabinet imported"},importNam{"+ IMPORT NAM"},importIr{"+ IMPORT WAV"},load{"DONE",ButtonKind::primary},close{"CLOSE"};
};}

