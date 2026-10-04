#include "AppShell.h"
#include "PmxTheme.h"

namespace pmx::ui
{
AppShell::AppShell()
{
    setWantsKeyboardFocus(true);
    brand.setText("PMX", juce::dontSendNotification);
    brand.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    brand.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    addAndMakeVisible(brand);

    const char* names[] { "LIVE", "LOOPER", "PRESETS", "SETTINGS" };
    for (std::size_t i = 0; i < nav.size(); ++i)
    {
        nav[i] = std::make_unique<PmxButton>(names[i]);
        addAndMakeVisible(*nav[i]);
    }
    nav[0]->onClick = [this] { select(Page::live); };
    nav[1]->onClick = [this] { select(Page::looper); };
    nav[2]->onClick = [this] { select(Page::presets); };
    nav[3]->onClick = [this] { select(Page::settings); };

    status.setText("PMX 0.2 alpha  |  OFFLINE", juce::dontSendNotification);
    status.setJustificationType(juce::Justification::centredRight);
    status.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    addAndMakeVisible(status);
    footer.setText("Pocket Master is audio I/O only | All processing stays in PMX | Audio stays on this computer",juce::dontSendNotification);
    footer.setColour(juce::Label::textColourId,juce::Colour(Theme::mutedText));
    footer.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(footer);
    addAndMakeVisible(update);
    addAndMakeVisible(liveScreen);
    addAndMakeVisible(presetsScreen);
    addAndMakeVisible(looperScreen);
    addAndMakeVisible(settingsScreen);
    addAndMakeVisible(updateDialog);
    addAndMakeVisible(performanceMode);
    addAndMakeVisible(setupWizard);
    presetsScreen.setVisible(false);
    looperScreen.setVisible(false);
    settingsScreen.setVisible(false);
    updateDialog.setVisible(false);
    performanceMode.setVisible(false);
    setupWizard.setVisible(true);
    setupWizard.toFront(false);
    update.onClick = [this] { updateDialog.setVisible(true); updateDialog.toFront(false); updateDialog.beginCheck(); };
    updateDialog.onDismiss = [this] { updateDialog.setVisible(false); };
    updateDialog.onInstallerReady = [](const std::filesystem::path& path) {
        if (pmx::update::JuceReleaseClient::launchInstaller(path))
            if (auto* app = juce::JUCEApplication::getInstance()) app->systemRequestedQuit();
    };
    liveScreen.onChooseSound = [this] { select(Page::presets); };
    liveScreen.onPerformanceMode = [this] { showPerformance(true); };
    performanceMode.onExit = [this] { showPerformance(false); };
    performanceMode.onShortcut=[this](input::ShortcutCommand command){if(command==input::ShortcutCommand::tuner){showPerformance(false);liveScreen.toggleTuner();}else if(onShortcut)onShortcut(command);};
    performanceMode.onEditEffect=[this](std::size_t index){showPerformance(false);liveScreen.editEffect(index);};
    performanceMode.onDelayChanged=[this](float t,float f,float m){if(liveScreen.onEffectChanged)liveScreen.onEffectChanged(7,{true,{t,f,m}});};
    liveScreen.onOpenLooper=[this]{select(Page::looper);};
    showSetup(true);
}

void AppShell::setTopStatus(const std::string& text, bool healthy)
{
    status.setText(juce::String(text), juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, juce::Colour(healthy ? Theme::healthy : Theme::mutedText));
}

void AppShell::showSetup(bool show)
{
    setupWizard.setVisible(show);
    for(auto& button:nav)button->setVisible(!show);
    update.setVisible(!show);
    if(show){liveScreen.setVisible(false);presetsScreen.setVisible(false);looperScreen.setVisible(false);settingsScreen.setVisible(false);}
    else select(page);
    if (show) setupWizard.toFront(false);
}

void AppShell::select(Page next)
{
    performanceActive = false;
    performanceMode.setVisible(false);
    page = next;
    for(std::size_t i=0;i<nav.size();++i)nav[i]->setToggleState(i==static_cast<std::size_t>(page),juce::dontSendNotification);
    liveScreen.setVisible(page == Page::live);
    presetsScreen.setVisible(page == Page::presets);
    looperScreen.setVisible(page == Page::looper);
    settingsScreen.setVisible(page == Page::settings);
    repaint();
}

void AppShell::showPerformance(bool show)
{
    performanceActive = show;
    performanceMode.setVisible(show);
    if (show)
    {
        liveScreen.setVisible(false); presetsScreen.setVisible(false); looperScreen.setVisible(false); settingsScreen.setVisible(false);
        performanceMode.toFront(false);
    }
    else
        select(page);
    repaint();
}

bool AppShell::keyPressed(const juce::KeyPress& key)
{
    pmx::input::ShortcutContext context;
    if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        context.textEntryFocused = dynamic_cast<juce::TextEditor*>(focused) != nullptr;
    context.modalDialogOpen = setupWizard.isVisible() || updateDialog.isVisible() || liveScreen.hasModalEditorOpen() || juce::Component::getCurrentlyModalComponent()!=nullptr;
    const auto command = pmx::input::ShortcutManager::commandFor(key.getKeyCode(), context);
    if (!command) return false;

    if (*command == pmx::input::ShortcutCommand::tuner && !performanceActive)
        liveScreen.toggleTuner();
    if (*command == pmx::input::ShortcutCommand::bypass)
    {
        bypassVisual = !bypassVisual;
        liveScreen.setBypassVisual(bypassVisual);
    }
    if (onShortcut) onShortcut(*command);
    return true;
}

juce::Rectangle<int> AppShell::contentBounds() const noexcept
{
    return getLocalBounds().withTrimmedTop(Theme::topBarHeight).withTrimmedBottom(24).reduced(Theme::pagePadding);
}

void AppShell::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(Theme::background));
    g.setColour(juce::Colour(Theme::topBar));
    g.fillRect(getLocalBounds().withHeight(Theme::topBarHeight));
    g.setColour(juce::Colour(Theme::border));
    g.drawHorizontalLine(Theme::topBarHeight - 1, 0.0f, static_cast<float>(getWidth()));
    g.setColour(juce::Colour(Theme::accent));const int heights[]{5,10,18,12,6};
    for(int i=0;i<5;++i)g.fillRoundedRectangle(static_cast<float>(14+i*4),static_cast<float>(Theme::topBarHeight/2-heights[i]/2),2.0f,static_cast<float>(heights[i]),1.0f);
}

void AppShell::resized()
{
    auto top = getLocalBounds().withHeight(Theme::topBarHeight).reduced(12, 8);
    auto brandArea=top.removeFromLeft(118);brandArea.removeFromLeft(28);brand.setBounds(brandArea);
    for (auto& button : nav)
    {
        button->setBounds(top.removeFromLeft(82).reduced(4, 0));
    }
    update.setBounds(top.removeFromRight(90).reduced(4, 0));
    status.setBounds(top.removeFromRight(180));
    footer.setBounds(14,getHeight()-23,getWidth()-28,22);
    liveScreen.setBounds(contentBounds());
    presetsScreen.setBounds(contentBounds());
    looperScreen.setBounds(contentBounds());
    settingsScreen.setBounds(contentBounds());
    updateDialog.setBounds(getLocalBounds());
    performanceMode.setBounds(contentBounds());
    setupWizard.setBounds(getLocalBounds().withTrimmedTop(Theme::topBarHeight).withTrimmedBottom(24));
}
} // namespace pmx::ui
