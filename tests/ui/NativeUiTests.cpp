#include "ui/AppShell.h"
#include <juce_graphics/juce_graphics.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "nam/NamProcessor.h"
#include "ir/IrProcessor.h"

namespace
{
bool checkBounds(juce::Component& component)
{
    bool ok=true;
    // Exercise the actually linked native engines, including static model registration.
    const auto modelFile=directory.getChildFile("linear-test.nam");
    modelFile.replaceWithText(R"({"version":"0.6.0","architecture":"Linear","config":{"receptive_field":1,"bias":false},"weights":[0.5],"sample_rate":48000})");
    pmx::nam::NamProcessor nam;
    const auto loaded=nam.load(std::filesystem::path(modelFile.getFullPathName().toStdString()),48000,128);
    if(!loaded.ok){std::cerr<<"Native NAM load failed: "<<loaded.error<<'\n';ok=false;}
    float samples[128]{};samples[0]=.5f;nam.process(samples,128);
    if(loaded.ok&&std::abs(samples[0]-.25f)>.002f){std::cerr<<"Native NAM did not process its generated linear fixture\n";ok=false;}
    const auto invalidFile=directory.getChildFile("invalid-test.nam");invalidFile.replaceWithText("invalid");
    if(nam.load(std::filesystem::path(invalidFile.getFullPathName().toStdString()),48000,128).ok){std::cerr<<"Invalid NAM was accepted\n";ok=false;}
    std::fill_n(samples,128,0.0f);samples[0]=.5f;nam.process(samples,128);
    if(loaded.ok&&std::abs(samples[0]-.25f)>.002f){std::cerr<<"Invalid NAM destroyed the working model\n";ok=false;}
    auto impulse=std::make_shared<pmx::ir::IrData>();impulse->sampleRate=48000;impulse->taps={1.0f,.5f};
    pmx::ir::IrProcessor ir;ir.setPrepared(impulse,128);std::fill_n(samples,128,0.0f);samples[0]=.5f;ir.process(samples,128);
    if(std::abs(samples[0]-.5f)>.002f||std::abs(samples[1]-.25f)>.002f){std::cerr<<"Prepared native convolution output is incorrect\n";ok=false;}
    modelFile.deleteFile();invalidFile.deleteFile();
    for(int i=0;i<component.getNumChildComponents();++i)
    {
        auto& child=*component.getChildComponent(i);
        if(!child.isVisible()) continue;
        // Composite slider internals include deliberately inset label borders.
        if(dynamic_cast<juce::Slider*>(&component)!=nullptr || dynamic_cast<juce::ComboBox*>(&component)!=nullptr || dynamic_cast<juce::TextEditor*>(&component)!=nullptr || dynamic_cast<juce::Viewport*>(&component)!=nullptr) continue;
        if(child.getWidth()<=0 || child.getHeight()<=0 || !component.getLocalBounds().contains(child.getBounds()))
        {
            std::cerr<<"Control outside its parent: "<<child.getName()<<" "<<child.getBounds().toString()<<" parent "<<component.getLocalBounds().toString()<<'\n';
            ok=false;
        }
        ok=checkBounds(child)&&ok;
    }
    return ok;
}
juce::Button* buttonNamed(juce::Component& parent,const juce::String& name)
{
    for(int i=0;i<parent.getNumChildComponents();++i)
    {
        auto* child=parent.getChildComponent(i);
        if(auto* button=dynamic_cast<juce::Button*>(child);button && button->getButtonText()==name) return button;
        if(auto* nested=buttonNamed(*child,name)) return nested;
    }
    return nullptr;
}
void render(juce::Component& component,const juce::File& file)
{
    auto snapshot=component.createComponentSnapshot(component.getLocalBounds(),true);
    file.deleteFile();
    juce::FileOutputStream output(file);
    juce::PNGImageFormat png;
    if(!png.writeImageToStream(snapshot,output)) throw std::runtime_error("PNG output failed");
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    const juce::File directory=argc>1?juce::File(juce::String(argv[1])):juce::File::getCurrentWorkingDirectory().getChildFile("ui-renders");
    directory.createDirectory();
    bool ok=true;
    pmx::ui::AppShell shell;
    int shortcutCalls=0;
    shell.onShortcut=[&](auto){++shortcutCalls;};
    shell.showSetup(true);
    shell.keyPressed(juce::KeyPress('r'));
    if(shortcutCalls!=0){std::cerr<<"First-run setup must block recording shortcuts\n";ok=false;}
    for(const auto size: {std::pair{1920,1080},std::pair{1366,768},std::pair{1100,680}})
    {
        shell.setSize(size.first,size.second);
        shell.showSetup(true);
        const auto prefix=juce::String(size.first)+"x"+juce::String(size.second)+"-";
        render(shell,directory.getChildFile(prefix+"welcome.png"));
        ok=checkBounds(shell)&&ok;
        shell.showSetup(false);
        const std::pair<pmx::ui::AppShell::Page,const char*> pages[]{
            {pmx::ui::AppShell::Page::live,"live"},{pmx::ui::AppShell::Page::looper,"looper"},
            {pmx::ui::AppShell::Page::presets,"presets"},{pmx::ui::AppShell::Page::settings,"settings"}};
        for(const auto& [page,name]:pages)
        {
            shell.goTo(page);
            render(shell,directory.getChildFile(prefix+name+".png"));
            ok=checkBounds(shell)&&ok;
        }
        shell.goTo(pmx::ui::AppShell::Page::live);
        if(auto* gate=buttonNamed(shell.live(),"GATE");gate && gate->onClick) gate->onClick();
        if(!shell.live().hasModalEditorOpen()){std::cerr<<"Gate card must open an effect editor\n";ok=false;}
        render(shell,directory.getChildFile(prefix+"gate.png"));
        ok=checkBounds(shell)&&ok;
        if(auto* done=buttonNamed(shell.live(),"DONE");done && done->onClick) done->onClick();
    }
    return ok?0:1;
}
