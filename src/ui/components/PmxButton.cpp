#include "PmxButton.h"
#include "../PmxTheme.h"
namespace pmx::ui {
PmxButton::PmxButton(juce::String text, ButtonKind kind):juce::TextButton(std::move(text)) {
 setMouseCursor(juce::MouseCursor::PointingHandCursor);
 setName(getButtonText());
 setColour(textColourOffId,juce::Colour(Theme::text));setColour(textColourOnId,juce::Colour(Theme::text));
 auto base=juce::Colour(Theme::panelRaised);
 if(kind==ButtonKind::primary)base=juce::Colour(Theme::accent);
 if(kind==ButtonKind::healthy)base=juce::Colour(Theme::healthy).darker(.35f);
 if(kind==ButtonKind::danger)base=juce::Colour(Theme::danger).darker(.65f);
 setColour(buttonColourId,base);setColour(buttonOnColourId,juce::Colour(Theme::accent).withAlpha(.20f));
}
void PmxButton::paintButton(juce::Graphics& g,bool over,bool down) {
 const bool card=getProperties().contains("subtitle");
 auto colour=findColour(getToggleState()?buttonOnColourId:buttonColourId);
 if(card)colour=juce::Colour(Theme::panel);
 if(getToggleState())colour=juce::Colour(Theme::accent).withAlpha(.20f);
 if(over)colour=colour.brighter(.10f);if(down)colour=colour.darker(.10f);
 auto r=getLocalBounds().toFloat().reduced(.5f);
 g.setColour(colour.withAlpha(isEnabled()?1.0f:.40f));g.fillRoundedRectangle(r,card?9.0f:6.0f);
 g.setColour(juce::Colour(getToggleState()?Theme::accent:Theme::border));g.drawRoundedRectangle(r,card?9.0f:6.0f,1);
 if(!card) {g.setColour(juce::Colour(Theme::text).withAlpha(isEnabled()?1.0f:.4f));g.setFont(juce::FontOptions(12.0f,juce::Font::bold));g.drawFittedText(getButtonText(),getLocalBounds().reduced(8,4),juce::Justification::centred,1);return;}
 auto text=getLocalBounds().reduced(14,12);auto category=getProperties()["category"].toString();
 if(category.isNotEmpty()){g.setColour(juce::Colour(Theme::mutedText));g.setFont(10.0f);g.drawText(category.toUpperCase(),text.removeFromTop(22),juce::Justification::centredLeft);}
 g.setColour(juce::Colour(Theme::text));g.setFont(juce::FontOptions(category.isEmpty()?12.0f:17.0f,juce::Font::bold));
 g.drawFittedText(getButtonText(),text.removeFromTop(24),juce::Justification::centredLeft,1);
 g.setColour(juce::Colour(Theme::mutedText));g.setFont(11.0f);g.drawFittedText(getProperties()["subtitle"].toString(),text.removeFromTop(30),juce::Justification::topLeft,2);
 if(getProperties().contains("active")){bool active=static_cast<bool>(getProperties()["active"]);float x=static_cast<float>(getWidth()-34),y=static_cast<float>(getHeight()-20);g.setColour(juce::Colour(active?Theme::healthy:Theme::mutedText).withAlpha(.25f));g.fillRoundedRectangle(x,y,22,10,5);g.setColour(juce::Colour(active?Theme::healthy:Theme::mutedText));g.fillEllipse(x+(active?13:2),y+2,6,6);g.setFont(9.0f);g.drawText(active?"ON":"OFF",14,getHeight()-24,30,18,juce::Justification::centredLeft);}
}
}

