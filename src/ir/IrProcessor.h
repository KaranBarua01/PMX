#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include "dsp/effects/Parameter.h"
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
    void swapPrepared(IrProcessor& other) noexcept;
    void setLowCutHz(float v) noexcept {lowCut=dsp::safeParameter(v,20,500,60);}
    void setHighCutHz(float v) noexcept {highCut=dsp::safeParameter(v,1000,20000,8000);}
    void setOutputDb(float v) noexcept {outputDb=dsp::safeParameter(v,-24,6,0);}
    void process(float* mono, int numSamples) noexcept;
    [[nodiscard]] bool loaded() const noexcept { return static_cast<bool>(data); }
private:
    std::shared_ptr<const IrData> data;
    std::vector<float> history;
    std::size_t write{0};
    std::atomic<float> lowCut{60},highCut{8000},outputDb{0};
    float lowState{},highState{};
};
}
