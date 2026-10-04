#include "NamIrBrowser.h"
#include "ui/PmxTheme.h"
namespace pmx::ui {
NamIrBrowser::NamIrBrowser(){
 for(auto* l:{&title,&subtitle,&ampLabel,&cabLabel,&hint}){addAndMakeVisible(*l);l->setColour(juce::Label::textColourId,juce::Colour(Theme::mutedText));}
 title.setText("Find your amp. Choose your cabinet.",juce::dontSendNotification);title.setFont(juce::FontOptions(24.0f,juce::Font::bold));title.setColour(juce::Label::textColourId,juce::Colour(Theme::text));
 subtitle.setText("Import local files. Your sounds stay on this computer.",juce::dontSendNotification);
 ampLabel.setText("AMP CAPTURES | NAM",juce::dontSendNotification);cabLabel.setText("CABINETS | IR",juce::dontSendNotification);
 hint.setText("No models or cabinets are bundled. Import files you have permission to use.",juce::dontSendNotification);
 for(auto* b:{&amp,&cab,&importNam,&importIr,&load,&close})addAndMakeVisible(*b);
 amp.getProperties().set("subtitle","Import a mono .nam model to begin");cab.getProperties().set("subtitle","Import a mono WAV cabinet impulse");
 amp.onClick=importNam.onClick=[this]{if(onImportNam)onImportNam();};cab.onClick=importIr.onClick=[this]{if(onImportIr)onImportIr();};close.onClick=load.onClick=[this]{if(onLoadSelected)onLoadSelected();};
}
void NamIrBrowser::paint(juce::Graphics& g){g.fillAll(juce::Colours::black.withAlpha(.72f));g.setColour(juce::Colour(Theme::panel));g.fillRoundedRectangle(card.toFloat(),12);g.setColour(juce::Colour(Theme::border));g.drawRoundedRectangle(card.toFloat().reduced(.5f),12,1);}
void NamIrBrowser::resized(){
 card=getLocalBounds().withSizeKeepingCentre(juce::jmin(900,juce::jmax(600,getWidth()-60)),460);auto r=card.reduced(26);
 auto heading=r.removeFromTop(36);close.setBounds(heading.removeFromRight(70));title.setBounds(heading);subtitle.setBounds(r.removeFromTop(28));r.removeFromTop(22);
 auto cols=r.removeFromTop(230);auto left=cols.removeFromLeft((cols.getWidth()-18)/2);cols.removeFromLeft(18);auto right=cols;
 auto h=left.removeFromTop(36);importNam.setBounds(h.removeFromRight(136));ampLabel.setBounds(h);h=right.removeFromTop(36);importIr.setBounds(h.removeFromRight(136));cabLabel.setBounds(h);
 left.removeFromTop(14);right.removeFromTop(14);amp.setBounds(left.removeFromTop(132));cab.setBounds(right.removeFromTop(132));hint.setBounds(r.removeFromTop(34));load.setBounds(r.removeFromBottom(38).removeFromRight(130));
}
}

