#pragma once
#include <string>

namespace pmx::audio
{
struct AudioDiagnosticInput
{
    bool connected { false };
    bool asio { false };
    std::string deviceName;
    double sampleRate { 0.0 };
    int bufferSize { 0 };
    int inputLatencySamples { 0 };
    int outputLatencySamples { 0 };
    std::string error;
};

struct AudioDiagnosticResult
{
    bool ok { false };
    std::string connectionText;
    std::string detail;
    std::string bufferText;
    std::string latencyText;
    double driverIoLatencyMs { 0.0 };
};

class AudioDiagnosticService final
{
public:
    static AudioDiagnosticResult evaluate(const AudioDiagnosticInput&);
};
} // namespace pmx::audio
