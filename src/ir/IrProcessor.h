#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
namespace pmx::ir
{
struct IrData { double sampleRate{0.0}; std::vector<float> taps; };
struct IrLoadResult { bool ok{false}; std::string error; std::shared_ptr<const IrData> data; };
class IrProcessor final
{
public:
    static IrLoadResult loadWav(const std::filesystem::path&, double targetSampleRate, std::size_t maxFrames);
    void setPrepared(std::shared_ptr<const IrData>);
    void reset() noexcept;
    void process(float* mono, int numSamples) noexcept;
    [[nodiscard]] bool loaded() const noexcept { return static_cast<bool>(data); }
private:
    std::shared_ptr<const IrData> data;
    std::vector<float> history;
    std::size_t write{0};
};
}
