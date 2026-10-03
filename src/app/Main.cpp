#include <juce_gui_extra/juce_gui_extra.h>
#include "pmx/AppVersion.h"
#include "ui/MainWindow.h"

namespace
{
class PmxApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "PMX"; }
    const juce::String getApplicationVersion() override
    {
        return juce::String(pmx::AppVersion::current().data());
    }
    bool moreThanOneInstanceAllowed() override { return true; }
    void initialise(const juce::String&) override { window = std::make_unique<pmx::ui::MainWindow>(); }
    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }
private:
    std::unique_ptr<pmx::ui::MainWindow> window;
};
}

START_JUCE_APPLICATION(PmxApplication)
