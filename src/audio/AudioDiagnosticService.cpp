#include "AudioDiagnosticService.h"
#include <iomanip>
#include <sstream>

namespace pmx::audio
{
AudioDiagnosticResult AudioDiagnosticService::evaluate(const AudioDiagnosticInput& in)
{
    AudioDiagnosticResult out;
    if (!in.connected || !in.asio)
    {
        out.ok = false;
        out.connectionText = "Pocket Master could not be opened.";
        out.detail = in.error.empty() ? "Check the USB connection and ASIO driver, then try again." : in.error;
        return out;
    }

    out.ok = true;
    out.connectionText = in.deviceName.empty() ? "Pocket Master connected" : in.deviceName + " connected";
    out.bufferText = std::to_string(in.bufferSize) + " samples";
    if (in.sampleRate > 0.0)
        out.driverIoLatencyMs = 1000.0 * static_cast<double>(in.inputLatencySamples + in.outputLatencySamples) / in.sampleRate;
    std::ostringstream latency;
    latency << std::fixed << std::setprecision(2) << out.driverIoLatencyMs << " ms driver-reported I/O latency";
    out.latencyText = latency.str();
    std::ostringstream detail;
    detail << std::fixed << std::setprecision(1) << (in.sampleRate / 1000.0) << " kHz • " << out.bufferText;
    out.detail = detail.str();
    return out;
}
} // namespace pmx::audio
