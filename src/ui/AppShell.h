#pragma once
#include <array>
#include <functional>
#include <string>
#include <juce_gui_basics/juce_gui_basics.h>
#include "components/PmxButton.h"
#include "LayoutPolicy.h"
#include "pmx/AppVersion.h"
#include "screens/LiveScreen.h"
#include "screens/PresetsScreen.h"
#include "screens/LooperScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/SpecialScreen.h"
#include "screens/FirstRunWizard.h"
#include "dialogs/UpdateDialog.h"
#include "screens/PerformanceMode.h"
#include "input/ShortcutManager.h"

namespace pmx::ui
{
class AppShell final : public juce::Component
{
public:
    enum class Page { live, looper, presets, special, settings };
    AppShell();
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    std::function<void(pmx::input::ShortcutCommand)> onShortcut;
    Page currentPage() const noexcept { return page; }
    juce::Rectangle<int> contentBounds() const noexcept;

    LiveScreen& live() noexcept { return liveScreen; }
    LooperScreen& looper() noexcept { return looperScreen; }
    PresetsScreen& presets() noexcept { return presetsScreen; }
    SettingsScreen& settings() noexcept { return settingsScreen; }
    SpecialScreen& special() noexcept { return specialScreen; }
    FirstRunWizard& setup() noexcept { return setupWizard; }
    PerformanceMode& performance() noexcept {return performanceMode;}
    UpdateDialog& updater() noexcept { return updateDialog; }
    void setTopStatus(const std::string& text, bool healthy);
    void showSetup(bool show);
    void goTo(Page next) { select(next); }

private:
    void select(Page);
    void showPerformance(bool);
    juce::Label brand;
    std::array<std::unique_ptr<PmxButton>, 5> nav;
    PmxButton update { "UPDATE" };
    juce::Label status;
    juce::Label footer;
    LiveScreen liveScreen;
    PresetsScreen presetsScreen;
    LooperScreen looperScreen;
    SettingsScreen settingsScreen;
    SpecialScreen specialScreen;
    UpdateDialog updateDialog { std::string(pmx::AppVersion::current()) };
    PerformanceMode performanceMode;
    FirstRunWizard setupWizard;
    bool performanceActive { false };
    bool bypassVisual { false };
    Page page { Page::live };
};
} // namespace pmx::ui
