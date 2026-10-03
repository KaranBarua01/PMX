#pragma once
#include <filesystem>
#include <memory>
#include <string>
namespace pmx::nam
{
struct NamLoadResult { bool ok{false}; std::string error; double expectedSampleRate{-1.0}; int inputChannels{0}; int outputChannels{0}; };
class NamProcessor final
{
public:
    NamProcessor(); ~NamProcessor(); NamProcessor(NamProcessor&&) noexcept; NamProcessor& operator=(NamProcessor&&) noexcept;
    NamProcessor(const NamProcessor&)=delete; NamProcessor& operator=(const NamProcessor&)=delete;
    NamLoadResult load(const std::filesystem::path&, double processingSampleRate, int maxBlockSize);
    void unload() noexcept;
    void setInputTrimDb(float db) noexcept; void setOutputTrimDb(float db) noexcept;
    void process(float* mono,int numSamples) noexcept;
    [[nodiscard]] bool loaded() const noexcept;
    [[nodiscard]] static bool engineCompiled() noexcept;
private: struct Impl; std::unique_ptr<Impl> impl;
};
}
