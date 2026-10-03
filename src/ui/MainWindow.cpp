#include "MainWindow.h"
#include "AppShell.h"
#include "PmxTheme.h"
#include "app/PmxRuntime.h"

namespace pmx::ui
{
MainWindow::MainWindow()
    : juce::DocumentWindow("PMX", juce::Colour(Theme::background), juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(Theme::minimumWindowWidth, Theme::minimumWindowHeight, 2560, 1600);
    auto* shell = new AppShell();
    setContentOwned(shell, true);
    runtime = std::make_unique<pmx::app::PmxRuntime>(*shell);
    centreWithSize(1366, 768);
    setVisible(true);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
} // namespace pmx::ui
