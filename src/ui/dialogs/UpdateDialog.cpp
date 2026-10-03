#include "UpdateDialog.h"
#include "ui/PmxTheme.h"
#include <chrono>
#include <juce_core/juce_core.h>

namespace pmx::ui
{
UpdateDialog::UpdateDialog(std::string currentVersion) : installedVersion(std::move(currentVersion))
{
    icon.setText("↗", juce::dontSendNotification); icon.setJustificationType(juce::Justification::centred); icon.setColour(juce::Label::textColourId, juce::Colour(Theme::accent)); icon.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    eyebrow.setText("PMX UPDATE", juce::dontSendNotification); eyebrow.setColour(juce::Label::textColourId, juce::Colour(Theme::accent)); eyebrow.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    title.setText("A little better. Still your sound.", juce::dontSendNotification); title.setColour(juce::Label::textColourId, juce::Colour(Theme::text)); title.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    subtitle.setText("Updates improve PMX without touching your presets, imported sounds, loops or recordings.", juce::dontSendNotification); subtitle.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    installedLabel.setText("INSTALLED  " + juce::String(installedVersion), juce::dontSendNotification); installedLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    availableLabel.setText("AVAILABLE  —", juce::dontSendNotification); availableLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::healthy));
    status.setText("Ready to check GitHub Releases.", juce::dontSendNotification); status.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));

    notes.setMultiLine(true); notes.setReadOnly(true); notes.setScrollbarsShown(true); notes.setText("What's new will appear here after an update check.");
    notes.setColour(juce::TextEditor::backgroundColourId, juce::Colour(Theme::panelRaised)); notes.setColour(juce::TextEditor::outlineColourId, juce::Colour(Theme::border)); notes.setColour(juce::TextEditor::textColourId, juce::Colour(Theme::text));

    for (auto* c : {static_cast<juce::Component*>(&icon),&eyebrow,&title,&subtitle,&installedLabel,&availableLabel,&status,&notes,&dismiss,&action}) addAndMakeVisible(*c);
    dismiss.onClick=[this]{ if(onDismiss)onDismiss(); else setVisible(false); };
    action.onClick=[this]{ if(!stagedInstaller.empty()){if(onInstallerReady)onInstallerReady(stagedInstaller);return;} if(decision.status==pmx::update::UpdateStatus::available)startDownload(); else beginCheck(); };
}

void UpdateDialog::beginCheck()
{
    if(checking||downloading)return;
    const auto version=pmx::update::SemanticVersion::parse(installedVersion);
    if(!version){status.setText("Installed PMX version could not be read.",juce::dontSendNotification);return;}
    const auto channel=version->prerelease.empty()?pmx::update::UpdateChannel::stable:pmx::update::UpdateChannel::preview;
    checking=true; stagedInstaller.clear(); action.setButtonText("CHECKING…"); action.setEnabled(false); status.setText("Checking GitHub Releases…",juce::dontSendNotification);
    checkFuture=pmx::update::ReleaseChecker::checkAsync(*version,channel,[](std::string& error){return pmx::update::JuceReleaseClient::fetchReleases(error);});
    startTimer(100);
}

void UpdateDialog::startDownload()
{
    if(downloading||decision.status!=pmx::update::UpdateStatus::available)return;
    const auto base=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("PMX").getChildFile("updates");
    const auto destination=base.getChildFile(juce::String(decision.release.assetName)).getFullPathName().toStdString();
    downloading=true; action.setButtonText("DOWNLOADING…"); action.setEnabled(false); status.setText("Downloading and verifying the installer…",juce::dontSendNotification);
    downloadFuture=pmx::update::JuceReleaseClient::downloadInstallerAsync(decision.release,std::filesystem::path(destination));
    startTimer(100);
}

void UpdateDialog::timerCallback()
{
    using namespace std::chrono_literals;
    if(checking && checkFuture.valid() && checkFuture.wait_for(0ms)==std::future_status::ready)
    {
        decision=checkFuture.get(); checking=false; updatePresentation();
    }
    if(downloading && downloadFuture.valid() && downloadFuture.wait_for(0ms)==std::future_status::ready)
    {
        const auto result=downloadFuture.get(); downloading=false;
        if(result.ok){stagedInstaller=result.path;status.setText("Verified installer is ready. Save any active take before installing.",juce::dontSendNotification);action.setButtonText("INSTALL & RESTART");action.setEnabled(true);}
        else{status.setText(juce::String(result.error),juce::dontSendNotification);action.setButtonText("TRY AGAIN");action.setEnabled(true);}
    }
    if(!checking&&!downloading)stopTimer();
}

void UpdateDialog::updatePresentation()
{
    action.setEnabled(true);
    if(decision.status==pmx::update::UpdateStatus::available)
    {
        availableLabel.setText("AVAILABLE  "+juce::String(decision.release.tagName),juce::dontSendNotification);
        notes.setText(decision.release.notes.empty()?"This release has no notes.":juce::String(decision.release.notes));
        status.setText("Update available. PMX will verify the SHA-256 digest before installation.",juce::dontSendNotification);
        action.setButtonText("UPDATE");
    }
    else if(decision.status==pmx::update::UpdateStatus::upToDate)
    {
        availableLabel.setText("✓ UP TO DATE",juce::dontSendNotification); notes.setText("You're running the newest release for this update channel."); status.setText("No update is required.",juce::dontSendNotification); action.setButtonText("CHECK AGAIN");
    }
    else
    {
        availableLabel.setText("OFFLINE",juce::dontSendNotification); notes.setText(juce::String(decision.message)); status.setText("Music-making still works offline.",juce::dontSendNotification); action.setButtonText("TRY AGAIN");
    }
}

void UpdateDialog::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(Theme::panel)); g.fillRoundedRectangle(getLocalBounds().toFloat(),Theme::cornerRadius);
    g.setColour(juce::Colour(Theme::border)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f),Theme::cornerRadius,1.0f);
}

void UpdateDialog::resized()
{
    auto r=getLocalBounds().reduced(24); icon.setBounds(r.removeFromTop(42).withWidth(42)); eyebrow.setBounds(r.removeFromTop(20)); title.setBounds(r.removeFromTop(38)); subtitle.setBounds(r.removeFromTop(44));
    auto versions=r.removeFromTop(30); installedLabel.setBounds(versions.removeFromLeft(220)); availableLabel.setBounds(versions); r.removeFromTop(10); notes.setBounds(r.removeFromTop(150)); r.removeFromTop(10); status.setBounds(r.removeFromTop(38));
    auto buttons=r.removeFromBottom(42); action.setBounds(buttons.removeFromRight(170)); buttons.removeFromRight(10); dismiss.setBounds(buttons.removeFromRight(110));
}
} // namespace pmx::ui
