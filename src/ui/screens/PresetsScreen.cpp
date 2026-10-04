#include "PresetsScreen.h"
#include "ui/PmxTheme.h"
namespace pmx::ui {
PresetsScreen::PresetsScreen(){
 for(auto* l:{&title,&subtitle,&empty}){addAndMakeVisible(*l);l->setColour(juce::Label::textColourId,juce::Colour(Theme::mutedText));}
 title.setText("A sound for every mood.",juce::dontSendNotification);title.setFont(juce::FontOptions(30.0f,juce::Font::bold));title.setColour(juce::Label::textColourId,juce::Colour(Theme::text));
 subtitle.setText("Choose a preset and start playing. Pin the ones you come back to.",juce::dontSendNotification);
 empty.setText("No sounds match. Try another category or search.",juce::dontSendNotification);
 search.setTextToShowWhenEmpty("Search your presets",juce::Colour(Theme::mutedText));search.setColour(juce::TextEditor::backgroundColourId,juce::Colour(Theme::panel));search.setColour(juce::TextEditor::outlineColourId,juce::Colour(Theme::border));search.setColour(juce::TextEditor::textColourId,juce::Colour(Theme::text));search.setIndents(12,0);addAndMakeVisible(search);search.onTextChange=[this]{rebuild();};
 addAndMakeVisible(savePreset);savePreset.onClick=[this]{if(onSavePreset)onSavePreset();};
 for(auto& c:categories){addAndMakeVisible(c);auto* button=&c;c.onClick=[this,button]{category=button->getButtonText();rebuild();};}
 addAndMakeVisible(viewport);viewport.setViewedComponent(&grid,false);viewport.setScrollBarsShown(true,false);
 setPresets(presets::factoryPresets());
}
void PresetsScreen::setPresets(std::vector<presets::Preset> p){library=std::move(p);rebuild();}
void PresetsScreen::setCurrent(const std::string& id){currentId=id;rebuild();}
void PresetsScreen::rebuild(){
 cards.clear();stars.clear();
 for(auto& c:categories)c.setToggleState(c.getButtonText()==category,juce::dontSendNotification);
 const auto query=search.getText().trim();
 for(const auto& p:library){
  if(category=="Favorites"&&!p.favorite)continue;
  if(category!="All sounds"&&category!="Favorites"&&!category.equalsIgnoreCase(p.category))continue;
  if(query.isNotEmpty()&&!juce::String(p.displayName).containsIgnoreCase(query)&&!juce::String(p.category).containsIgnoreCase(query))continue;
  auto card=std::make_unique<PmxButton>(juce::String(p.displayName));card->getProperties().set("category",p.category);card->getProperties().set("subtitle",p.id==currentId?"● CURRENT SOUND":"Guitar · Starting sound");card->setToggleState(p.id==currentId,juce::dontSendNotification);
  const auto id=p.id;card->onClick=[this,id]{if(onPresetChosen)onPresetChosen(id);};grid.addAndMakeVisible(*card);cards.push_back(std::move(card));
  auto star=std::make_unique<PmxButton>(p.favorite?"★":"☆");star->onClick=[this,id]{for(auto& item:library)if(item.id==id){item.favorite=!item.favorite;if(onFavoriteChanged)onFavoriteChanged(id,item.favorite);break;}rebuild();};grid.addAndMakeVisible(*star);stars.push_back(std::move(star));
 }
 empty.setVisible(cards.empty());resized();repaint();
}
void PresetsScreen::paint(juce::Graphics& g){g.fillAll(juce::Colour(Theme::background));}
void PresetsScreen::resized(){
 auto r=getLocalBounds();auto header=r.removeFromTop(86);savePreset.setBounds(header.removeFromRight(144).removeFromTop(36));title.setBounds(header.removeFromTop(42));subtitle.setBounds(header.removeFromTop(26));
 auto side=r.removeFromLeft(164);for(auto& c:categories)c.setBounds(side.removeFromTop(43).reduced(0,4));r.removeFromLeft(22);
 search.setBounds(r.removeFromTop(38));r.removeFromTop(14);viewport.setBounds(r);empty.setBounds(r.withHeight(44));
 const int gap=12,columns=4,width=(juce::jmax(0,viewport.getWidth()-16)-gap*3)/columns;
 const int height=juce::jlimit(136,190,(r.getHeight()-gap*2)/3);
 const int rows=static_cast<int>((cards.size()+3)/4);grid.setSize(juce::jmax(0,viewport.getWidth()-16),juce::jmax(r.getHeight(),rows*(height+gap)-gap));
 for(std::size_t i=0;i<cards.size();++i){int x=static_cast<int>(i%4)*(width+gap),y=static_cast<int>(i/4)*(height+gap);cards[i]->setBounds(x,y,width,height);stars[i]->setBounds(x+width-34,y+8,26,26);}
}
}

