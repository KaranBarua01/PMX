#include <cmath>
#include "audio/AudioDiagnosticService.h"

int main()
{
    pmx::audio::AudioDiagnosticInput input;
    input.connected=true; input.asio=true; input.deviceName="Pocket Master";
    input.sampleRate=48000.0; input.bufferSize=128; input.inputLatencySamples=64; input.outputLatencySamples=64;
    const auto healthy=pmx::audio::AudioDiagnosticService::evaluate(input);
    if(!healthy.ok) return 1;
    if(healthy.connectionText!="Pocket Master connected") return 2;
    if(std::abs(healthy.driverIoLatencyMs-2.6666667)>0.01) return 3;
    if(healthy.bufferText.find("128 samples")==std::string::npos) return 4;
    if(healthy.latencyText.find("driver-reported") == std::string::npos) return 5;

    pmx::audio::AudioDiagnosticInput missing;
    missing.connected=false; missing.error="Device busy";
    const auto failed=pmx::audio::AudioDiagnosticService::evaluate(missing);
    if(failed.ok) return 6;
    if(failed.connectionText!="Pocket Master could not be opened.") return 7;
    if(failed.detail.find("Device busy")==std::string::npos) return 8;
    return 0;
}
