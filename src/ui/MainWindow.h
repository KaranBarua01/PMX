#pragma once
#include <memory>
#include <juce_gui_basics/juce_gui_basics.h>

namespace pmx::app { class PmxRuntime; }

namespace pmx::ui
{
class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow();
    ~MainWindow() override;
    void closeButtonPressed() override;
private:
    std::unique_ptr<pmx::app::PmxRuntime> runtime;
};
} // namespace pmx::ui
