#include "PresetsScreen.h"
#include "ui/PmxTheme.h"

namespace pmx::ui
{
PresetsScreen::PresetsScreen()
{
    title.setText("A sound for every mood.", juce::dontSendNotification);
    title.setFont(juce::FontOptions(30.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    addAndMakeVisible(title);

    subtitle.setText("Choose a starting point, then make it yours.", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    addAndMakeVisible(subtitle);

    search.setTextToShowWhenEmpty("Search by name or mood", juce::Colour(Theme::mutedText));
    search.setColour(juce::TextEditor::backgroundColourId, juce::Colour(Theme::panel));
    search.setColour(juce::TextEditor::outlineColourId, juce::Colour(Theme::border));
    search.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(Theme::accent));
    search.setColour(juce::TextEditor::textColourId, juce::Colour(Theme::text));
    search.setIndents(12, 0);
    addAndMakeVisible(search);
    addAndMakeVisible(savePreset);
    savePreset.onClick = [this] { if (onSavePreset) onSavePreset(); };

    for (auto& category : categories) addAndMakeVisible(category);
    for (auto& card : cards)
    {
        addAndMakeVisible(card);
        card.onClick = [this, &card] { if (onPresetChosen) onPresetChosen(card.getButtonText().toStdString()); };
    }
}

void PresetsScreen::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(Theme::background));
    const auto bounds = getLocalBounds().reduced(24);
    const auto body = bounds.withTrimmedTop(112);
    const auto side = body.withWidth(180);
    g.setColour(juce::Colour(Theme::panel));
    g.fillRoundedRectangle(side.toFloat(), static_cast<float>(Theme::cornerRadius));
    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(side.toFloat(), static_cast<float>(Theme::cornerRadius), 1.0f);
}

void PresetsScreen::resized()
{
    auto r = getLocalBounds().reduced(24);
    auto heading = r.removeFromTop(92);
    savePreset.setBounds(heading.removeFromRight(130).removeFromTop(38));
    title.setBounds(heading.removeFromTop(42));
    subtitle.setBounds(heading.removeFromTop(28));

    auto body = r;
    auto side = body.removeFromLeft(180).reduced(12);
    for (auto& category : categories)
        category.setBounds(side.removeFromTop(42).reduced(0, 3));

    body.removeFromLeft(18);
    auto filters = body.removeFromTop(48);
    search.setBounds(filters.removeFromLeft(380).reduced(0, 4));
    body.removeFromTop(12);

    constexpr int columns = 4;
    constexpr int rows = 3;
    const int gap = 12;
    const int cardWidth = (body.getWidth() - gap * (columns - 1)) / columns;
    const int cardHeight = juce::jmax(90, (body.getHeight() - gap * (rows - 1)) / rows);
    for (std::size_t i = 0; i < cards.size(); ++i)
    {
        const int col = static_cast<int>(i) % columns;
        const int row = static_cast<int>(i) / columns;
        cards[i].setBounds(body.getX() + col * (cardWidth + gap),
                           body.getY() + row * (cardHeight + gap),
                           cardWidth, cardHeight);
    }
}
} // namespace pmx::ui
