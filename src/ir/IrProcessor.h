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
    IrProcessor();
    ~IrProcessor();
    static IrLoadResult loadWav(const std::filesystem::path&, double targetSampleRate, std::size_t maxFrames);
    void setPrepared(std::shared_ptr<const IrData>,int maxBlock=128);
    void reset() noexcept;
    void swapPrepared(IrProcessor& other) noexcept;
    void setLowCutHz(float v) noexcept {lowCut=dsp::safeParameter(v,20,500,60);filtersEnabled=true;}
    void setHighCutHz(float v) noexcept {highCut=dsp::safeParameter(v,1000,20000,8000);filtersEnabled=true;}
    void setOutputDb(float v) noexcept {outputDb=dsp::safeParameter(v,-24,6,0);filtersEnabled=true;}
    void process(float* mono, int numSamples) noexcept;
    [[nodiscard]] bool loaded() const noexcept { return static_cast<bool>(data); }
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    std::shared_ptr<const IrData> data;
    std::vector<float> history;
    std::size_t write{0};
    std::atomic<float> lowCut{60},highCut{8000},outputDb{0};
    std::atomic<bool> filtersEnabled{false};
    float lowState{},highState{};
};
}
