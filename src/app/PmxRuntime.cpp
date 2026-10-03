#include "PmxRuntime.h"
#include "RuntimePolicy.h"
#include "ir/IrProcessor.h"
#include "pmx/AppVersion.h"
#include <algorithm>
#include <cctype>
#include <chrono>

namespace pmx::app
{
namespace
{
bool isActiveLoop(looper::LooperState state)
{
    return state==looper::LooperState::recording || state==looper::LooperState::overdubbing || state==looper::LooperState::playing;
}

void showInfo(const juce::String& title,const juce::String& message,juce::MessageBoxIconType icon=juce::MessageBoxIconType::InfoIcon)
{
    juce::AlertWindow::showMessageBoxAsync(icon,title,message);
}
}

PmxRuntime::PmxRuntime(ui::AppShell& ui)
    : shell(ui), presetStore(appDataDirectory()/"presets"), tunerWindow(4096,0.0f)
{
    audioHost.setSink(&engine);
    engine.setMuted(true);
    wireUi();
    shell.setTopStatus(std::string("PMX ")+std::string(pmx::AppVersion::current())+"  •  OFFLINE",false);
    startTimerHz(20);
}

PmxRuntime::~PmxRuntime()
{
    stopTimer();
    engine.setMuted(true);
    if(engine.recorder().isRecording()) engine.recorder().stop();
    audioHost.close();
}

std::filesystem::path PmxRuntime::appDataDirectory() const
{
    const auto root=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("PMX");
    root.createDirectory();
    return std::filesystem::path(root.getFullPathName().toStdString());
}

void PmxRuntime::wireUi()
{
    shell.setup().onDiscoverDevice=[this]{discoverAndOpen();};
    shell.setup().onTestAudio=[this]{enableMonitoring(); if(connected) shell.setup().setAudioTestPassed(true);};
    shell.setup().onFinished=[this]{shell.showSetup(false);};

    shell.settings().onApply=[this]{
        engine.setInputGainDb(shell.settings().inputGainDb());
        engine.setOutputGainDb(shell.settings().outputGainDb());
        applySelection(shell.settings().selectedAudioSetup(),monitoring);
    };
    shell.settings().onTestAudio=[this]{enableMonitoring();};
    shell.settings().onTryAgain=[this]{discoverAndOpen();};

    shell.live().onBypassChanged=[this](bool enabled){bypassed=enabled;engine.setFxBypass(enabled);};
    shell.live().onSavePreset=[this]{saveCurrentPreset();};
    shell.live().onDelayChanged=[this](float timeMs,float feedback,float mix){
        auto& delay=engine.rack().delayEffect();
        delay.setTimeMs(timeMs); delay.setFeedback(feedback/100.0f); delay.setMix(mix/100.0f);
        engine.rack().setEnabled(dsp::RackModule::delay,true);
    };
    shell.live().onTapTempo=[this]{tapTempo();};
    shell.live().onImportNam=[this]{chooseNam();};
    shell.live().onImportIr=[this]{chooseIr();};

    shell.presets().onSavePreset=[this]{saveCurrentPreset();};
    shell.presets().onPresetChosen=[this](const std::string& name){choosePreset(name);};

    shell.looper().onRecord=[this]{engine.looper().record();};
    shell.looper().onPlay=[this]{engine.looper().play();};
    shell.looper().onOverdub=[this]{engine.looper().overdub();};
    shell.looper().onStop=[this]{engine.looper().stop();};
    shell.looper().onUndo=[this]{engine.looper().undo();};
    shell.looper().onRedo=[this]{engine.looper().redo();};
    shell.looper().onClear=[this]{
        if(juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,"Clear loop?","This removes the current loop from memory.","CLEAR","CANCEL"))
            engine.looper().clear();
    };
    shell.looper().onSaveLoop=[this]{saveLoopToLibrary();};
    shell.looper().onExportWav=[this]{exportLoop();};

    shell.onShortcut=[this](input::ShortcutCommand command){handleShortcut(command);};
    shell.updater().onInstallerReady=[this](const std::filesystem::path& path){installUpdate(path);};
}

void PmxRuntime::discoverAndOpen()
{
    auto devices=audioHost.scan();
    deviceController.updateAvailableDevices(devices);
    shell.settings().setAvailableDevices(devices);
    const auto preferred=RuntimePolicy::preferredPocketMaster(devices);
    if(!preferred)
    {
        connected=false; monitoring=false; engine.setMuted(true);
        shell.setup().setDetectedStatus(false,false,false);
        shell.live().setStatusText("● POCKET MASTER NOT FOUND",false);
        shell.setTopStatus("PMX 0.2 α  •  POCKET MASTER NOT FOUND",false);
        shell.settings().setConnectionStatus(false,"POCKET MASTER NOT FOUND","Connect Pocket Master over USB and confirm its ASIO driver is installed.",0.0);
        showInfo("Pocket Master not found","PMX could not find a Pocket Master ASIO device. Connect it by USB, close other apps using the driver, then try again.",juce::MessageBoxIconType::WarningIcon);
        return;
    }
    applySelection(*preferred,false);
    if(connected) shell.setup().setDetectedStatus(true,true,true);
}

bool PmxRuntime::applySelection(const audio::AudioDeviceSelection& selection,bool enableMonitor)
{
    const auto devices=audioHost.scan();
    deviceController.updateAvailableDevices(devices);
    if(!deviceController.open(selection))
    {
        connected=false; monitoring=false; engine.setMuted(true);
        const auto& status=deviceController.status();
        shell.live().setStatusText("● AUDIO CONFIGURATION ERROR",false);
        shell.settings().setConnectionStatus(false,"POCKET MASTER COULD NOT BE OPENED",status.message,0.0);
        return false;
    }

    engine.setMuted(true);
    audioHost.close();
    audioHost.setSink(&engine);
    const auto error=audioHost.open(selection);
    if(error.isNotEmpty())
    {
        connected=false; monitoring=false; deviceController.close(); engine.setMuted(true);
        shell.live().setStatusText("● POCKET MASTER COULD NOT BE OPENED",false);
        shell.settings().setConnectionStatus(false,"POCKET MASTER COULD NOT BE OPENED",error.toStdString(),0.0);
        return false;
    }

    currentSelection=selection;
    connected=true;
    monitoring=enableMonitor;
    deviceController.setMonitoringEnabled(enableMonitor);
    engine.setMuted(!enableMonitor);
    engine.setInputGainDb(shell.settings().inputGainDb());
    engine.setOutputGainDb(shell.settings().outputGainDb());
    shell.live().setStatusText("● POCKET MASTER CONNECTED    "+std::to_string(static_cast<int>(selection.sampleRate/1000.0))+" kHz • "+std::to_string(selection.bufferSize)+" samples",true);
    shell.setTopStatus("PMX 0.2 α  •  CONNECTED",true);
    updateDiagnostics();
    return true;
}

void PmxRuntime::enableMonitoring()
{
    if(!connected)
    {
        discoverAndOpen();
        if(!connected) return;
    }
    monitoring=true;
    deviceController.setMonitoringEnabled(true);
    engine.setMuted(false);
    shell.live().setStatusText("● POCKET MASTER CONNECTED    MONITORING ON",true);
}

void PmxRuntime::updateDiagnostics()
{
    audio::AudioDiagnosticInput input;
    input.connected=connected;
    input.asio=connected;
    if(currentSelection)
    {
        input.deviceName=currentSelection->deviceName;
        input.sampleRate=currentSelection->sampleRate;
        input.bufferSize=currentSelection->bufferSize;
    }
    if(auto* device=audioHost.deviceManager().getCurrentAudioDevice())
    {
        input.inputLatencySamples=device->getInputLatencyInSamples();
        input.outputLatencySamples=device->getOutputLatencyInSamples();
    }
    const auto result=audio::AudioDiagnosticService::evaluate(input);
    shell.settings().setConnectionStatus(result.ok,result.connectionText,result.detail,result.driverIoLatencyMs);
}

void PmxRuntime::tapTempo()
{
    engine.tempo().tap(juce::Time::getMillisecondCounterHiRes()/1000.0);
}

void PmxRuntime::chooseNam()
{
    chooser=std::make_unique<juce::FileChooser>("Choose a NAM model",juce::File{},"*.nam");
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& fc){
        const auto file=fc.getResult(); if(file.existsAsFile()) loadNam(std::filesystem::path(file.getFullPathName().toStdString()));
    });
}

void PmxRuntime::chooseIr()
{
    chooser=std::make_unique<juce::FileChooser>("Choose a cabinet IR",juce::File{},"*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& fc){
        const auto file=fc.getResult(); if(file.existsAsFile()) loadIr(std::filesystem::path(file.getFullPathName().toStdString()));
    });
}

void PmxRuntime::loadNam(const std::filesystem::path& path)
{
    if(isActiveLoop(engine.looper().state())){showInfo("Stop the loop first","Stop playback/recording before changing the NAM model.");return;}
    const bool resume=monitoring;
    engine.setMuted(true); audioHost.close();
    const auto sr=currentSelection?currentSelection->sampleRate:44100.0;
    const auto block=currentSelection?currentSelection->bufferSize:128;
    const auto result=engine.nam().load(path,sr,block);
    if(currentSelection) audioHost.open(*currentSelection);
    engine.setMuted(!resume);
    if(!result.ok){showInfo("NAM could not be loaded",juce::String(result.error),juce::MessageBoxIconType::WarningIcon);return;}
    currentNamPath=path; engine.rack().setEnabled(dsp::RackModule::nam,true); shell.live().setNamName(path.stem().string());
}

void PmxRuntime::loadIr(const std::filesystem::path& path)
{
    if(isActiveLoop(engine.looper().state())){showInfo("Stop the loop first","Stop playback/recording before changing the cabinet IR.");return;}
    const bool resume=monitoring;
    engine.setMuted(true); audioHost.close();
    const auto sr=currentSelection?currentSelection->sampleRate:44100.0;
    const auto loaded=ir::IrProcessor::loadWav(path,sr,8192);
    if(loaded.ok) engine.ir().setPrepared(loaded.data);
    if(currentSelection) audioHost.open(*currentSelection);
    engine.setMuted(!resume);
    if(!loaded.ok){showInfo("IR could not be loaded",juce::String(loaded.error),juce::MessageBoxIconType::WarningIcon);return;}
    currentIrPath=path; engine.rack().setEnabled(dsp::RackModule::ir,true); shell.live().setIrName(path.stem().string());
}

std::string PmxRuntime::slugify(const std::string& value)
{
    std::string out;
    for(unsigned char c:value)
    {
        if(std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
        else if((c==' '||c=='-'||c=='_')&&!out.empty()&&out.back()!='-') out.push_back('-');
    }
    while(!out.empty()&&out.back()=='-') out.pop_back();
    return out.empty()?"preset":out;
}

void PmxRuntime::saveCurrentPreset()
{
    presets::Preset p;
    p.id=slugify(currentPresetName); p.displayName=currentPresetName; p.category="User";
    const auto names=dsp::GuitarRack::moduleNames();
    for(unsigned i=0;i<static_cast<unsigned>(dsp::RackModule::count);++i)
        p.modules[std::string(names[i])]=engine.rack().isEnabled(static_cast<dsp::RackModule>(i));
    p.parameters["delay.timeMs"]=shell.live().delayTimeMs();
    p.parameters["delay.feedbackPercent"]=shell.live().delayFeedbackPercent();
    p.parameters["delay.mixPercent"]=shell.live().delayMixPercent();
    if(!currentNamPath.empty()){p.nam.id=currentNamPath.stem().string();p.nam.path=currentNamPath;}
    if(!currentIrPath.empty()){p.ir.id=currentIrPath.stem().string();p.ir.path=currentIrPath;}
    const auto result=presetStore.save(p);
    if(result.ok) showInfo("Preset saved",juce::String(p.displayName)+" is saved in your PMX library.");
    else showInfo("Preset could not be saved",juce::String(result.error),juce::MessageBoxIconType::WarningIcon);
}

void PmxRuntime::choosePreset(const std::string& name)
{
    currentPresetName=name; shell.live().setPresetName(name); shell.goTo(ui::AppShell::Page::live);
    const auto loaded=presetStore.load(slugify(name));
    if(loaded.ok) applyPreset(loaded.preset);
}

void PmxRuntime::applyPreset(const presets::Preset& preset)
{
    const auto names=dsp::GuitarRack::moduleNames();
    for(unsigned i=0;i<static_cast<unsigned>(dsp::RackModule::count);++i)
    {
        const auto it=preset.modules.find(std::string(names[i]));
        if(it!=preset.modules.end()) engine.rack().setEnabled(static_cast<dsp::RackModule>(i),it->second);
    }
    auto value=[&](const char* key,double fallback){const auto it=preset.parameters.find(key);return it==preset.parameters.end()?fallback:it->second;};
    auto& delay=engine.rack().delayEffect();
    delay.setTimeMs(static_cast<float>(value("delay.timeMs",420.0)));
    delay.setFeedback(static_cast<float>(value("delay.feedbackPercent",23.0)/100.0));
    delay.setMix(static_cast<float>(value("delay.mixPercent",24.0)/100.0));
    currentPresetName=preset.displayName; shell.live().setPresetName(currentPresetName);
    if(!preset.nam.path.empty()&&std::filesystem::exists(preset.nam.path)) loadNam(preset.nam.path);
    if(!preset.ir.path.empty()&&std::filesystem::exists(preset.ir.path)) loadIr(preset.ir.path);
}

void PmxRuntime::saveLoopToLibrary()
{
    if(engine.looper().state()!=looper::LooperState::stopped){showInfo("Stop the loop first","Saved loops are written only from the Stopped state.");return;}
    const auto dir=appDataDirectory()/"loops"; std::error_code ec; std::filesystem::create_directories(dir,ec);
    const auto stamp=juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S").toStdString();
    loopWrite=looper::LoopExportService::writeWav24Async(engine.looper().snapshot(),dir/("PMX-Loop-"+stamp+".wav"));
}

void PmxRuntime::exportLoop()
{
    if(engine.looper().state()!=looper::LooperState::stopped){showInfo("Stop the loop first","Export is available when the loop is stopped.");return;}
    chooser=std::make_unique<juce::FileChooser>("Export PMX loop",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("PMX-Loop.wav"),"*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& fc){
        auto file=fc.getResult(); if(file==juce::File{}) return; if(file.getFileExtension().isEmpty()) file=file.withFileExtension("wav");
        loopWrite=looper::LoopExportService::writeWav24Async(engine.looper().snapshot(),std::filesystem::path(file.getFullPathName().toStdString()));
    });
}

void PmxRuntime::toggleQuickRecord()
{
    if(engine.recorder().isRecording())
    {
        const auto result=engine.recorder().stop();
        showInfo(result.ok?"Recording saved":"Recording problem",result.ok?juce::String(result.path.string()):juce::String(result.error),result.ok?juce::MessageBoxIconType::InfoIcon:juce::MessageBoxIconType::WarningIcon);
        return;
    }
    const auto base=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("PMX Recordings"); base.createDirectory();
    const auto name="PMX-Take-"+juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S")+".wav";
    const auto result=engine.recorder().start(std::filesystem::path(base.getChildFile(name).getFullPathName().toStdString()));
    if(!result.ok) showInfo("Recording could not start",juce::String(result.error),juce::MessageBoxIconType::WarningIcon);
}

void PmxRuntime::handleShortcut(input::ShortcutCommand command)
{
    switch(command)
    {
        case input::ShortcutCommand::bypass: bypassed=!bypassed; engine.setFxBypass(bypassed); shell.live().setBypassVisual(bypassed); break;
        case input::ShortcutCommand::mute: monitoring=!monitoring; engine.setMuted(!monitoring); deviceController.setMonitoringEnabled(monitoring); break;
        case input::ShortcutCommand::tuner: break;
        case input::ShortcutCommand::loopTransport:
            switch(engine.looper().state())
            {
                case looper::LooperState::empty: case looper::LooperState::stopped: engine.looper().record(); break;
                case looper::LooperState::recording: engine.looper().play(); break;
                case looper::LooperState::playing: engine.looper().overdub(); break;
                case looper::LooperState::overdubbing: engine.looper().play(); break;
            }
            break;
        case input::ShortcutCommand::stop: engine.looper().stop(); break;
        case input::ShortcutCommand::tapTempo: tapTempo(); break;
        case input::ShortcutCommand::quickRecord: toggleQuickRecord(); break;
    }
}

juce::String PmxRuntime::looperStateText(looper::LooperState state)
{
    switch(state){case looper::LooperState::empty:return "NEW LOOP";case looper::LooperState::recording:return "RECORDING";case looper::LooperState::playing:return "PLAYING";case looper::LooperState::overdubbing:return "OVERDUBBING";case looper::LooperState::stopped:return "STOPPED";} return "LOOP";
}

void PmxRuntime::updateLooperUi()
{
    const auto& l=engine.looper(); shell.looper().setTransportState(looperStateText(l.state())); shell.looper().setTiming(l.recordedFrames(),l.loopFrames(),engine.sampleRate());
}

void PmxRuntime::installUpdate(const std::filesystem::path& path)
{
    if(isActiveLoop(engine.looper().state())||engine.recorder().isRecording())
    {
        showInfo("Finish the active take first","Stop the loop and Quick Recorder before installing an update.",juce::MessageBoxIconType::WarningIcon); return;
    }
    engine.setMuted(true); audioHost.close();
    if(update::JuceReleaseClient::launchInstaller(path))
        if(auto* application=juce::JUCEApplication::getInstance()) application->systemRequestedQuit();
}

void PmxRuntime::timerCallback()
{
    if(connected && audioHost.deviceManager().getCurrentAudioDevice()==nullptr)
    {
        connected=false; monitoring=false; engine.setMuted(true); deviceController.notifyDisconnected();
        shell.live().setStatusText("● POCKET MASTER DISCONNECTED",false); shell.setTopStatus("PMX 0.2 α  •  DISCONNECTED",false);
    }

    if(shell.live().tunerVisible() && engine.sampleRate()>0.0)
    {
        const int count=engine.tunerTap().pop(tunerWindow.data(),static_cast<int>(tunerWindow.size()));
        if(count>=256) shell.live().setTunerResult(tunerEngine.analyse(tunerWindow.data(),count,engine.sampleRate()));
    }
    updateLooperUi();
    if(++diagnosticsCountdown>=20){diagnosticsCountdown=0;updateDiagnostics();}
    if(loopWrite.valid() && loopWrite.wait_for(std::chrono::milliseconds(0))==std::future_status::ready)
    {
        const auto result=loopWrite.get();
        showInfo(result.ok?"Loop saved":"Loop save failed",result.ok?juce::String(result.path.string()):juce::String(result.error),result.ok?juce::MessageBoxIconType::InfoIcon:juce::MessageBoxIconType::WarningIcon);
    }
}
} // namespace pmx::app
