#pragma once

namespace pmx::state
{
enum class SetupStep { welcome, deviceFound, ready };

class SetupState final
{
public:
    [[nodiscard]] SetupStep step() const noexcept { return currentStep; }
    [[nodiscard]] bool inputReady() const noexcept { return input; }
    [[nodiscard]] bool outputReady() const noexcept { return output; }
    [[nodiscard]] bool asioReady() const noexcept { return asio; }
    [[nodiscard]] bool audioTestPassed() const noexcept { return audioTest; }

    void setDeviceStatus(bool inputOk, bool outputOk, bool asioOk) noexcept
    {
        input = inputOk;
        output = outputOk;
        asio = asioOk;
        if (input && output && asio)
            currentStep = SetupStep::deviceFound;
        else
        {
            currentStep = SetupStep::welcome;
            audioTest = false;
        }
    }

    void setAudioTestPassed(bool passed) noexcept
    {
        audioTest = passed && input && output && asio;
    }

    bool continueToReady() noexcept
    {
        if (currentStep != SetupStep::deviceFound || !audioTest)
            return false;
        currentStep = SetupStep::ready;
        return true;
    }

    [[nodiscard]] bool canFinish() const noexcept
    {
        return currentStep == SetupStep::ready && input && output && asio && audioTest;
    }

    void reset() noexcept
    {
        currentStep = SetupStep::welcome;
        input = output = asio = audioTest = false;
    }

private:
    SetupStep currentStep { SetupStep::welcome };
    bool input { false };
    bool output { false };
    bool asio { false };
    bool audioTest { false };
};
} // namespace pmx::state
