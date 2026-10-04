#include "PmxKnob.h"
#include "../PmxTheme.h"
#include <cmath>
namespace pmx::ui {
namespace {
class KnobLook final:public juce::LookAndFeel_V4 {
public:
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider&) override {
  float radius=juce::jmin(width,height)*.40f,cx=x+width*.5f,cy=y+height*.5f;
  g.setColour(juce::Colour(Theme::panelRaised));g.fillEllipse(cx-radius,cy-radius,radius*2,radius*2);
  juce::Path track;track.addCentredArc(cx,cy,radius,radius,0,start,end,true);g.setColour(juce::Colour(Theme::border));g.strokePath(track,juce::PathStrokeType(2));
  float angle=start+position*(end-start);juce::Path arc;arc.addCentredArc(cx,cy,radius,radius,0,start,angle,true);g.setColour(juce::Colour(Theme::accent));g.strokePath(arc,juce::PathStrokeType(2.5f));
  g.setColour(juce::Colour(Theme::text));g.drawLine(cx+std::sin(angle)*radius*.56f,cy-std::cos(angle)*radius*.56f,cx+std::sin(angle)*radius*.80f,cy-std::cos(angle)*radius*.80f,2);
 }
};
KnobLook& knobLook(){static KnobLook look;return look;}
}
PmxKnob::PmxKnob(juce::String suffix){
 setLookAndFeel(&knobLook());setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,24);setTextValueSuffix(std::move(suffix));
 setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(Theme::accent));setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(Theme::border));setColour(juce::Slider::textBoxTextColourId,juce::Colour(Theme::text));setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
}
}
