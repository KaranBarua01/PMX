#include "FirstRunWizard.h"
#include "ui/PmxTheme.h"

namespace pmx::ui
{
FirstRunWizard::FirstRunWizard()
{
    setOpaque(false);
    for (auto* c : { static_cast<juce::Component*>(&stepIndicator), &icon, &title, &subtitle, &deviceSummary, &hint, &primary, &secondary })
        addAndMakeVisible(*c);

    stepIndicator.setJustificationType(juce::Justification::centred);
    stepIndicator.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    icon.setJustificationType(juce::Justification::centred);
    icon.setColour(juce::Label::textColourId, juce::Colour(Theme::accent));
    icon.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centred);
    title.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    title.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    subtitle.setFont(juce::FontOptions(13.0f));
    deviceSummary.setJustificationType(juce::Justification::centred);
    deviceSummary.setColour(juce::Label::textColourId, juce::Colour(Theme::healthy));
    hint.setJustificationType(juce::Justification::centred);
    hint.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    hint.setFont(juce::FontOptions(11.0f));

    primary.onClick = [this] { continuePressed(); };
    secondary.onClick = [this]
    {
        if (setup.step() == state::SetupStep::deviceFound && onTestAudio)
            onTestAudio();
    };
    refresh();
}

void FirstRunWizard::setDetectedStatus(bool inputOk, bool outputOk, bool asioOk)
{
    setup.setDeviceStatus(inputOk, outputOk, asioOk);
    refresh();
}

void FirstRunWizard::setAudioTestPassed(bool passed)
{
    setup.setAudioTestPassed(passed);
    refresh();
}

void FirstRunWizard::continuePressed()
{
    if (setup.step() == state::SetupStep::welcome)
    {
        if (onDiscoverDevice) onDiscoverDevice();
        return;
    }
    if (setup.step() == state::SetupStep::deviceFound)
    {
        setup.continueToReady();
        refresh();
        return;
    }
    if (setup.canFinish() && onFinished)
        onFinished();
}

void FirstRunWizard::refresh()
{
    const auto s = setup.step();
    if (s == state::SetupStep::welcome)
    {
        stepIndicator.setText("●  SET UP     ○  CHECK AUDIO     ○  PLAY", juce::dontSendNotification);
        icon.setText("◉", juce::dontSendNotification);
        title.setText("WELCOME TO PMX", juce::dontSendNotification);
        subtitle.setText("Plug your guitar into Pocket Master and connect Pocket Master to this PC.", juce::dontSendNotification);
        deviceSummary.setText("Guitar  →  Pocket Master  →  USB  →  This PC", juce::dontSendNotification);
        primary.setButtonText("CONTINUE");
        secondary.setVisible(false);
        hint.setText("PMX will keep the Pocket Master as audio I/O only.", juce::dontSendNotification);
    }
    else if (s == state::SetupStep::deviceFound)
    {
        stepIndicator.setText("○  SET UP     ●  CHECK AUDIO     ○  PLAY", juce::dontSendNotification);
        icon.setText("✓", juce::dontSendNotification);
        title.setText("POCKET MASTER FOUND", juce::dontSendNotification);
        subtitle.setText("Input and output are ready. Test the return path before playing.", juce::dontSendNotification);
        deviceSummary.setText("INPUT ✓      OUTPUT ✓      ASIO ✓", juce::dontSendNotification);
        secondary.setButtonText("TEST AUDIO");
        secondary.setVisible(true);
        primary.setButtonText(setup.audioTestPassed() ? "CONTINUE" : "CONTINUE");
        primary.setEnabled(setup.audioTestPassed());
        hint.setText(setup.audioTestPassed() ? "Audio test passed." : "PMX starts with monitoring muted for safety.", juce::dontSendNotification);
    }
    else
    {
        stepIndicator.setText("○  SET UP     ○  CHECK AUDIO     ●  PLAY", juce::dontSendNotification);
        icon.setText("✓", juce::dontSendNotification);
        title.setText("YOU'RE READY", juce::dontSendNotification);
        subtitle.setText("Your PMX audio foundation is ready for the first playing test.", juce::dontSendNotification);
        deviceSummary.setText("DREAM CLEAN", juce::dontSendNotification);
        secondary.setVisible(false);
        primary.setEnabled(true);
        primary.setButtonText("START PLAYING");
        hint.setText("You can change audio settings any time from Settings.", juce::dontSendNotification);
    }
    if (s == state::SetupStep::welcome) primary.setEnabled(true);
    resized();
}

void FirstRunWizard::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(Theme::panel));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Theme::cornerRadius + 2.0f);
    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Theme::cornerRadius + 2.0f, 1.0f);
}

void FirstRunWizard::resized()
{
    auto r = getLocalBounds().reduced(42, 26);
    stepIndicator.setBounds(r.removeFromTop(28));
    r.removeFromTop(16);
    icon.setBounds(r.removeFromTop(44));
    title.setBounds(r.removeFromTop(40));
    subtitle.setBounds(r.removeFromTop(46));
    deviceSummary.setBounds(r.removeFromTop(42));
    r.removeFromTop(10);
    auto buttons = r.removeFromTop(38);
    if (secondary.isVisible())
    {
        auto left = buttons.removeFromLeft(buttons.getWidth() / 2).reduced(4, 0);
        secondary.setBounds(left);
        primary.setBounds(buttons.reduced(4, 0));
    }
    else
    {
        primary.setBounds(buttons.withSizeKeepingCentre(160, 38));
    }
    hint.setBounds(r.removeFromTop(32));
}
} // namespace pmx::ui
