#include "TunerView.h"
#include "ui/PmxTheme.h"
namespace pmx::ui
{
void TunerView::setResult(const analysis::TunerResult&r){result=r;repaint();}
void TunerView::paint(juce::Graphics&g){g.setColour(juce::Colour(Theme::panelRaised));g.fillRoundedRectangle(getLocalBounds().toFloat(),8);g.setColour(juce::Colour(Theme::text));g.setFont(juce::FontOptions(22.0f,juce::Font::bold));const auto note=result.confidence>0.2f?juce::String(result.noteName):"—";g.drawText(note,10,6,70,30,juce::Justification::centredLeft);g.setFont(12.0f);g.setColour(juce::Colour(Theme::mutedText));juce::String info=result.confidence>0.2f?juce::String(result.frequencyHz,1)+" Hz   "+juce::String(result.cents,1)+" cents":"Play one note";g.drawText(info,80,8,getWidth()-90,28,juce::Justification::centredLeft);if(result.confidence>0.2f){const float x=juce::jmap(juce::jlimit(-50.0f,50.0f,result.cents),-50.0f,50.0f,12.0f,static_cast<float>(getWidth()-12));g.setColour(std::abs(result.cents)<5.0f?juce::Colour(Theme::healthy):juce::Colour(Theme::accent));g.fillRect(x-2.0f,42.0f,4.0f,22.0f);}}
}
