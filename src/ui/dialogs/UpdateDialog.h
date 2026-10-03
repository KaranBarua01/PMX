#pragma once
#include <filesystem>
#include <functional>
#include <future>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"
#include "update/JuceReleaseClient.h"

namespace pmx::ui
{
class UpdateDialog final : public juce::Component, private juce::Timer
{
public:
    explicit UpdateDialog(std::string currentVersion);
    void beginCheck();
    void paint(juce::Graphics&) override;
    void resized() override;
    std::function<void(const std::filesystem::path&)> onInstallerReady;
    std::function<void()> onDismiss;

private:
    void timerCallback() override;
    void startDownload();
    void updatePresentation();

    std::string installedVersion;
    pmx::update::UpdateDecision decision;
    std::future<pmx::update::UpdateDecision> checkFuture;
    std::future<pmx::update::DownloadResult> downloadFuture;
    bool checking { false };
    bool downloading { false };
    std::filesystem::path stagedInstaller;

    juce::Label icon, eyebrow, title, subtitle, installedLabel, availableLabel, status;
    juce::TextEditor notes;
    PmxButton dismiss { "NOT NOW" };
    PmxButton action { "CHECK FOR UPDATE", ButtonKind::primary };
};
} // namespace pmx::ui
