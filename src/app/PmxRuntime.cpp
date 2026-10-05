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
    guitarProfile=instruments::GuitarProfile::loadOrStandard(appDataDirectory()/"guitar-profile.txt");
    engine.setGuitarOpenStringFrequencies(guitarProfile.openStrings());
    wireUi();
    applyPreset(presets::factoryPresets()[4]);
    refreshPresets();
    shell.setTopStatus(std::string("PMX ")+std::string(pmx::AppVersion::current())+"  •  OFFLINE",false);
    startTimerHz(20);
}

PmxRuntime::~PmxRuntime()
{
    stopTimer();
    engine.setMuted(true);
    if(engine.recorder().isRecording()) engine.recorder().stop();
    audioHost.close();
    if(assetLoad.valid())assetLoad.wait();
}

std::filesystem::path PmxRuntime::appDataDirectory() const
{
    const auto root=juce::File(juce::SystemStats::getEnvironmentVariable("LOCALAPPDATA",juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getFullPathName())).getChildFile("PMX");
    root.createDirectory();
    return std::filesystem::path(root.getFullPathName().toStdString());
}

void PmxRuntime::wireUi()
{
    shell.setup().onDiscoverDevice=[this]{discoverAndOpen();};
    shell.setup().onTestAudio=[this]{enableMonitoring(); if(connected&&monitoring) shell.setup().setAudioTestPassed(true);};
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
    shell.live().onEffectChanged=[this](std::size_t i,const presets::EffectSettings& value){sound.effects[i]=value;applyEffect(i,value);};
    shell.live().onMuteChanged=[this](bool mute){if(mute){monitoring=false;engine.setMuted(true);deviceController.setMonitoringEnabled(false);}else enableMonitoring();shell.live().setMutedVisual(!monitoring);};
    shell.live().onMetronomeChanged=[this](bool enabled){engine.setMetronomeEnabled(enabled);};
    shell.live().onMetronomeLevel=[this](float level){engine.setMetronomeLevel(level);};
    shell.live().onRhythmChanged=[this](bool enabled){engine.setRhythmEnabled(enabled);};
    shell.live().onRhythmLevel=[this](float level){engine.setRhythmLevel(level);};
    shell.live().onRhythmPattern=[this](int pattern){engine.setRhythmPattern(pattern);};
    shell.live().onStringDrumsChanged=[this](bool enabled){engine.setStringDrumsEnabled(enabled);};
    shell.live().onStringDrumsLevel=[this](float level){engine.setStringDrumsLevel(level);};
    shell.live().onCalibrateGuitar=[this]{
        if(guitarCalibration.active()){
            guitarCalibration.cancel();
            shell.live().setGuitarCalibrationProgress(false,-1,false);
            return;
        }
        if(!connected){
            showInfo("Connect Pocket Master","Connect the Pocket Master before learning the guitar tuning.",juce::MessageBoxIconType::WarningIcon);
            return;
        }
        guitarCalibration.start(guitarProfile);
        shell.live().setGuitarCalibrationProgress(true,guitarCalibration.currentString(),false);
    };
    shell.live().onTempoChanged=[this](double bpm){engine.tempo().setBpm(bpm);};
    shell.live().onQuickRecord=[this]{toggleQuickRecord();};
    shell.looper().onLoopLevel=[this](float level){engine.setLoopLevel(level);};
    shell.live().onTapTempo=[this]{tapTempo();};
    shell.live().onImportNam=[this]{chooseNam();};
    shell.live().onImportIr=[this]{chooseIr();};

    shell.presets().onFavoriteChanged=[this](const std::string& id,bool favorite){for(auto p:presetLibrary)if(p.id==id){p.favorite=favorite;presetStore.save(p);break;}};
    shell.presets().onSavePreset=[this]{saveCurrentPreset();};
    shell.presets().onPresetChosen=[this](const std::string& name){choosePreset(name);};

    shell.looper().onRecord=[this]{loopCommand(audio::LoopCommand::record);};
    shell.looper().onPlay=[this]{loopCommand(audio::LoopCommand::play);};
    shell.looper().onOverdub=[this]{loopCommand(audio::LoopCommand::overdub);};
    shell.looper().onStop=[this]{loopCommand(audio::LoopCommand::stop);};
    shell.looper().onUndo=[this]{loopCommand(audio::LoopCommand::undo);};
    shell.looper().onRedo=[this]{loopCommand(audio::LoopCommand::redo);};
    shell.looper().onClear=[this]{
        const auto options=juce::MessageBoxOptions::makeOptionsOkCancel(
            juce::MessageBoxIconType::WarningIcon,
            "Clear loop?",
            "This removes the current loop from memory.",
            "CLEAR",
            "CANCEL",
            &shell);
        juce::AlertWindow::showAsync(options,[this](int result){ if(result==1) loopCommand(audio::LoopCommand::clear); });
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
    if(assetLoad.valid()||engine.recorder().isRecording()||isActiveLoop(engine.loopStatus().state)||engine.loopStatus().pendingCommands){showInfo("Finish the current take first","Stop the loop and Quick Recorder before changing audio settings.");return false;}
    const auto devices=audioHost.scan();
    shell.settings().setAvailableDevices(devices);
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
    shell.settings().setSelection(selection);
    connected=true;
    monitoring=false;
    enableMonitor=false;
    deviceController.setMonitoringEnabled(enableMonitor);
    engine.setMuted(!enableMonitor);
    engine.setInputGainDb(shell.settings().inputGainDb());
    engine.setOutputGainDb(shell.settings().outputGainDb());
    shell.live().setStatusText("● POCKET MASTER CONNECTED    "+std::to_string(static_cast<int>(selection.sampleRate/1000.0))+" kHz • "+std::to_string(selection.bufferSize)+" samples",true);
    shell.setTopStatus("PMX 0.2 α  •  CONNECTED",true);
    shell.live().setMutedVisual(true);
    applyPreset(sound.toPreset(currentPresetId,currentPresetName,currentCategory));
    updateDiagnostics();
    return true;
}

void PmxRuntime::enableMonitoring()
{
    if(assetLoad.valid()){showInfo("Sound loading","Wait for the sound to finish loading before enabling monitoring.");return;}
    if(!connected)
    {
        discoverAndOpen();
        if(!connected) return;
    }
    monitoring=true;
    shell.live().setMutedVisual(false);
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
    if(auto* device=audioHost.currentDevice())
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
    if(assetLoad.valid()){showInfo("Sound loading","Wait for the current import to finish.");return;}
    const auto sr=currentSelection?currentSelection->sampleRate:44100.0;
    const auto block=currentSelection?currentSelection->bufferSize:128;
    assetLoad=std::async(std::launch::async,[path,sr,block]{
        PreparedAsset result;result.path=path;result.rate=sr;result.block=block;
        result.nam=std::make_unique<nam::NamProcessor>();
        auto loaded=result.nam->load(path,sr,block);
        if(!loaded.ok){result.error=loaded.error;result.nam.reset();}
        return result;
    });
}
void PmxRuntime::loadIr(const std::filesystem::path& path)
{
    if(assetLoad.valid()){showInfo("Sound loading","Wait for the current import to finish.");return;}
    const auto sr=currentSelection?currentSelection->sampleRate:44100.0;
    const auto block=currentSelection?currentSelection->bufferSize:128;
    assetLoad=std::async(std::launch::async,[path,sr,block]{
        PreparedAsset result;result.path=path;result.rate=sr;result.block=block;
        auto loaded=ir::IrProcessor::loadWav(path,sr,8192);
        if(!loaded.ok)result.error=loaded.error;
        else{result.ir=std::make_unique<ir::IrProcessor>();result.ir->setPrepared(std::move(loaded.data),block);}
        return result;
    });
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

void PmxRuntime::refreshPresets()
{
    presetLibrary=presets::factoryPresets();
    for(const auto& saved:presetStore.list()){
        const auto it=std::find_if(presetLibrary.begin(),presetLibrary.end(),[&](const auto& p){return p.id==saved.id;});
        if(it!=presetLibrary.end())*it=saved;else presetLibrary.push_back(saved);
    }
    shell.presets().setPresets(presetLibrary);
}
void PmxRuntime::saveCurrentPreset()
{
    auto* dialog=new juce::AlertWindow("Save your sound","Give this sound a name.",juce::MessageBoxIconType::NoIcon,&shell);
    dialog->addTextEditor("name",juce::String(currentPresetName),"Name");
    dialog->addButton("SAVE",1);dialog->addButton("CANCEL",0);
    juce::Component::SafePointer<juce::AlertWindow> safe(dialog);
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([this,safe](int result){
        if(result!=1||safe==nullptr)return;
        const auto name=safe->getTextEditorContents("name").trim().toStdString();
        if(name.empty())return;
        auto p=sound.toPreset("user-"+slugify(name),name,"Special");
        const auto saved=presetStore.save(p);
        if(saved.ok){currentPresetName=name;shell.live().setPresetName(name);refreshPresets();shell.presets().setCurrent(p.id);}
        else showInfo("Preset could not be saved",juce::String(saved.error),juce::MessageBoxIconType::WarningIcon);
    }),true);
}
void PmxRuntime::choosePreset(const std::string& id)
{
    const auto it=std::find_if(presetLibrary.begin(),presetLibrary.end(),[&](const auto& p){return p.id==id;});
    if(it==presetLibrary.end())return;
    applyPreset(*it);shell.goTo(ui::AppShell::Page::live);
}
void PmxRuntime::applyEffect(std::size_t i,const presets::EffectSettings& settings)
{
    auto& rack=engine.rack();const auto& v=settings.values;
    rack.setEnabled(static_cast<dsp::RackModule>(i),settings.enabled);
    switch(i){
        case 0:rack.gateEffect().setThresholdDb(v[0]);rack.gateEffect().setAttackMs(v[1]);rack.gateEffect().setReleaseMs(v[2]);break;
        case 1:rack.compressorEffect().setThresholdDb(v[0]);rack.compressorEffect().setRatio(v[1]);rack.compressorEffect().setMakeupDb(v[2]);break;
        case 2:rack.driveEffect().setAmount(v[0]/100);rack.driveEffect().setTone(v[1]/100);rack.driveEffect().setOutputDb(v[2]);break;
        case 3:engine.nam().setInputTrimDb(v[0]);engine.nam().setOutputTrimDb(v[1]);break;
        case 4:engine.ir().setLowCutHz(v[0]);engine.ir().setHighCutHz(v[1]);engine.ir().setOutputDb(v[2]);break;
        case 5:rack.eqEffect().setLowDb(v[0]);rack.eqEffect().setMidDb(v[1]);rack.eqEffect().setHighDb(v[2]);break;
        case 6:rack.chorusEffect().setRateHz(v[0]);rack.chorusEffect().setDepth(v[1]/100);rack.chorusEffect().setMix(v[2]/100);break;
        case 7:rack.delayEffect().setTimeMs(v[0]);rack.delayEffect().setFeedback(v[1]/100);rack.delayEffect().setMix(v[2]/100);break;
        case 8:rack.reverbEffect().setDecay(v[0]/100);rack.reverbEffect().setTone(v[1]/100);rack.reverbEffect().setMix(v[2]/100);break;
    }
}
void PmxRuntime::commitPreset(const presets::Preset& preset)
{
    sound=presets::SoundState::fromPreset(preset);
    for(std::size_t i=0;i<9;++i)applyEffect(i,sound.effects[i]);
    currentPresetName=preset.displayName;currentPresetId=preset.id;currentCategory=preset.category;
    currentNamPath=preset.nam.path;currentIrPath=preset.ir.path;
    shell.live().setPresetName(currentPresetName);shell.live().setSoundState(sound);shell.presets().setCurrent(preset.id);
    shell.live().setNamName(preset.nam.path.empty()?"IMPORT AN AMP":preset.nam.path.stem().string());
    shell.live().setIrName(preset.ir.path.empty()?"IMPORT A CABINET":preset.ir.path.stem().string());
}
void PmxRuntime::applyPreset(const presets::Preset& preset)
{
    if(assetLoad.valid()){showInfo("Sound loading","Wait for the current import to finish before choosing another preset.");return;}
    const auto missing=presetStore.missingAssets(preset);
    if(!missing.empty()){juce::String warning;for(const auto& item:missing)warning+=juce::String(item)+"\n";showInfo("Preset needs a file",warning+"Your current sound is unchanged. Import the missing file and try again.",juce::MessageBoxIconType::WarningIcon);return;}
    const auto sr=currentSelection?currentSelection->sampleRate:44100.0;
    const auto block=currentSelection?currentSelection->bufferSize:128;
    assetLoad=std::async(std::launch::async,[preset,sr,block]{
        PreparedAsset result;result.preset=preset;result.rate=sr;result.block=block;
        result.nam=std::make_unique<nam::NamProcessor>();
        result.ir=std::make_unique<ir::IrProcessor>();
        if(!preset.nam.path.empty()){const auto loaded=result.nam->load(preset.nam.path,sr,block);if(!loaded.ok){result.error="Amp: "+loaded.error;return result;}}
        if(!preset.ir.path.empty()){auto loaded=ir::IrProcessor::loadWav(preset.ir.path,sr,8192);if(!loaded.ok){result.error="Cabinet: "+loaded.error;return result;}result.ir->setPrepared(std::move(loaded.data),block);}
        return result;
    });
}
void PmxRuntime::loopCommand(audio::LoopCommand command)
{
    if(!connected){showInfo("Connect Pocket Master","The live looper needs an open Pocket Master ASIO connection.");return;}
    if(!monitoring && (command==audio::LoopCommand::record||command==audio::LoopCommand::play||command==audio::LoopCommand::overdub)){showInfo("Enable monitoring","Choose UNMUTE before starting the loop.");return;}
    if(!engine.requestLoopCommand(command))showInfo("Looper busy","Wait a moment and try again.");
}
looper::LoopSnapshot PmxRuntime::snapshotLoop()
{
    const auto state=engine.loopStatus();
    if(state.pendingCommands||state.state!=looper::LooperState::stopped)return {};
    audioHost.suspend();
    auto snapshot=engine.looper().state()==looper::LooperState::stopped?engine.looper().snapshot():looper::LoopSnapshot{};
    if(connected&&!audioHost.resume()){connected=false;monitoring=false;engine.setMuted(true);}
    return snapshot;
}

void PmxRuntime::saveLoopToLibrary()
{
    if(engine.loopStatus().pendingCommands||engine.loopStatus().state!=looper::LooperState::stopped){showInfo("Stop the loop first","Saved loops are written only from the Stopped state.");return;}
    const auto dir=appDataDirectory()/"loops"; std::error_code ec; std::filesystem::create_directories(dir,ec);
    const auto stamp=juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S").toStdString();
    loopWrite=looper::LoopExportService::writeWav24Async(snapshotLoop(),dir/("PMX-Loop-"+stamp+".wav"));
}

void PmxRuntime::exportLoop()
{
    if(engine.loopStatus().pendingCommands||engine.loopStatus().state!=looper::LooperState::stopped){showInfo("Stop the loop first","Export is available when the loop is stopped.");return;}
    chooser=std::make_unique<juce::FileChooser>("Export PMX loop",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("PMX-Loop.wav"),"*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& fc){
        auto file=fc.getResult(); if(file==juce::File{}) return; if(file.getFileExtension().isEmpty()) file=file.withFileExtension("wav");
        loopWrite=looper::LoopExportService::writeWav24Async(snapshotLoop(),std::filesystem::path(file.getFullPathName().toStdString()));
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
    if(!connected||!monitoring){showInfo("Enable monitoring","Connect Pocket Master and choose UNMUTE before recording.");return;}
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
        case input::ShortcutCommand::mute: if(monitoring){monitoring=false;engine.setMuted(true);deviceController.setMonitoringEnabled(false);}else enableMonitoring();shell.live().setMutedVisual(!monitoring);break;
        case input::ShortcutCommand::tuner: break;
        case input::ShortcutCommand::loopTransport:
            switch(engine.loopStatus().state)
            {
                case looper::LooperState::empty: loopCommand(audio::LoopCommand::record); break;
                case looper::LooperState::stopped: loopCommand(audio::LoopCommand::play); break;
                case looper::LooperState::recording: loopCommand(audio::LoopCommand::play); break;
                case looper::LooperState::playing: loopCommand(audio::LoopCommand::overdub); break;
                case looper::LooperState::overdubbing: loopCommand(audio::LoopCommand::play); break;
            }
            break;
        case input::ShortcutCommand::stop: loopCommand(audio::LoopCommand::stop); break;
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
    const auto status=engine.loopStatus();
    shell.looper().setTransportState(looperStateText(status.state));
    shell.looper().setTiming(status.state==looper::LooperState::recording?status.recordedFrames:status.position,status.loopFrames,engine.sampleRate());
    shell.looper().setAvailability(connected&&monitoring,status.loopFrames>0,status.state==looper::LooperState::stopped&&!status.pendingCommands,status.canUndo,status.canRedo);
    shell.looper().setWaveform(engine.loopWaveform(),status.loopFrames>0?static_cast<float>(status.position)/status.loopFrames:0);
    if(engine.consumeLoopError())showInfo("Looper action unavailable","Finish or clear the current take before starting another. If overdub history is full, save the loop and start a new one.");
}

void PmxRuntime::installUpdate(const std::filesystem::path& path)
{
    if(assetLoad.valid()||engine.loopStatus().pendingCommands||isActiveLoop(engine.loopStatus().state)||engine.recorder().isRecording())
    {
        showInfo("Finish the active take first","Stop the loop and Quick Recorder before installing an update.",juce::MessageBoxIconType::WarningIcon); return;
    }
    engine.setMuted(true); audioHost.close();
    if(update::JuceReleaseClient::launchInstaller(path))
        if(auto* application=juce::JUCEApplication::getInstance()) application->systemRequestedQuit();
}

void PmxRuntime::timerCallback()
{
    if(engine.recorder().needsFinalisation()){
        const auto result=engine.recorder().stop();
        showInfo("Recording stopped",juce::String(result.error)+"\nPartial take: "+juce::String(result.path.string()),juce::MessageBoxIconType::WarningIcon);
    }
    if(connected && !audioHost.isRunning())
    {
        connected=false; monitoring=false; engine.setMuted(true);audioHost.close();if(engine.recorder().isRecording())engine.recorder().interruptForRecovery();deviceController.notifyDisconnected();shell.live().setMutedVisual(true);
        if(guitarCalibration.active()){guitarCalibration.cancel();shell.live().setGuitarCalibrationProgress(false,-1,false);}
        shell.live().setStatusText("● POCKET MASTER DISCONNECTED",false); shell.setTopStatus("PMX 0.2 α  •  DISCONNECTED",false);
    }

    if(connected && engine.sampleRate()>0.0)
    {
        const int count=engine.tunerTap().pop(tunerWindow.data(),static_cast<int>(tunerWindow.size()));
        if(count>=256 && (shell.live().tunerVisible() || guitarCalibration.active()))
        {
            const auto result=tunerEngine.analyse(tunerWindow.data(),count,engine.sampleRate());
            if(shell.live().tunerVisible()) shell.live().setTunerResult(result);
            if(guitarCalibration.active() && guitarCalibration.submit(result))
            {
                if(guitarCalibration.completed())
                {
                    guitarProfile=guitarCalibration.profile();
                    engine.setGuitarOpenStringFrequencies(guitarProfile.openStrings());
                    const auto saved=guitarProfile.save(appDataDirectory()/"guitar-profile.txt");
                    shell.live().setGuitarCalibrationProgress(false,-1,true);
                    showInfo(saved?"Guitar learned":"Guitar learned for this session",
                             saved?"PMX saved your six-string tuning. Future pitch and instrument features can use this fretboard profile."
                                  :"PMX learned the tuning, but could not save the profile file.",
                             saved?juce::MessageBoxIconType::InfoIcon:juce::MessageBoxIconType::WarningIcon);
                }
                else
                    shell.live().setGuitarCalibrationProgress(true,guitarCalibration.currentString(),false);
            }
        }
    }
    if(assetLoad.valid()&&assetLoad.wait_for(std::chrono::milliseconds(0))==std::future_status::ready){
        try {
            auto loaded=assetLoad.get();
            if(!loaded.error.empty())showInfo("Sound could not be loaded",juce::String(loaded.error)+"\nYour current sound is unchanged.",juce::MessageBoxIconType::WarningIcon);
            else if(currentSelection&&(loaded.rate!=currentSelection->sampleRate||loaded.block!=currentSelection->bufferSize))showInfo("Audio setup changed","Import the sound again using the new audio settings.");
            else {
                audioHost.suspend();
                if(loaded.preset){
                    engine.nam()=std::move(*loaded.nam);engine.ir().swapPrepared(*loaded.ir);
                    commitPreset(*loaded.preset);
                }else{
                    if(loaded.nam){engine.nam()=std::move(*loaded.nam);currentNamPath=loaded.path;sound.nam={loaded.path.stem().string(),loaded.path};sound.effects[3].enabled=true;applyEffect(3,sound.effects[3]);shell.live().setNamName(loaded.path.stem().string());}
                    if(loaded.ir){engine.ir().swapPrepared(*loaded.ir);currentIrPath=loaded.path;sound.ir={loaded.path.stem().string(),loaded.path};sound.effects[4].enabled=true;applyEffect(4,sound.effects[4]);shell.live().setIrName(loaded.path.stem().string());}
                    shell.live().setSoundState(sound);
                }
                if(connected&&!audioHost.resume()){connected=false;monitoring=false;engine.setMuted(true);}
            }
        }catch(const std::exception& e){showInfo("Sound import failed",juce::String(e.what()),juce::MessageBoxIconType::WarningIcon);}
    }
    const auto levels=engine.metrics();shell.live().setMeters(connected?levels.inputPeak:0,connected?levels.outputPeak:0);shell.live().setStringDrumDetected(engine.stringDrumsEnabled()?engine.lastStringDrum():-1);
    shell.performance().setState(currentPresetName,connected?"POCKET MASTER CONNECTED":"POCKET MASTER OFFLINE",connected,!monitoring,bypassed,connected?levels.inputPeak:0,connected?levels.outputPeak:0,engine.tempo().bpm());
    shell.performance().setDelay(sound.effects[7].values[0],sound.effects[7].values[1],sound.effects[7].values[2]);
    shell.live().setTempo(engine.tempo().bpm());shell.live().setRecordingVisual(engine.recorder().isRecording());
    if(!connected)shell.live().setTunerResult({});
    updateLooperUi();
    if(++diagnosticsCountdown>=20){diagnosticsCountdown=0;updateDiagnostics();}
    if(loopWrite.valid() && loopWrite.wait_for(std::chrono::milliseconds(0))==std::future_status::ready)
    {
        const auto result=loopWrite.get();
        showInfo(result.ok?"Loop saved":"Loop save failed",result.ok?juce::String(result.path.string()):juce::String(result.error),result.ok?juce::MessageBoxIconType::InfoIcon:juce::MessageBoxIconType::WarningIcon);
    }
}
} // namespace pmx::app
